function Start-Profiling {
    param (
        [Parameter()]
        [ValidateSet('Low', 'Medium', 'High')]
        [string]$SamplingRate = 'Medium',

        [Parameter()]
        [string]$OutputName = 'profile',

        [Parameter()]
        [string[]]$PmcProfile = @(),

        [Parameter()]
        [string[]]$PmcCount = @(),

        [Parameter(Position = 0, ValueFromRemainingArguments)]
        [string[]]$Command,

        [parameter()]
        [switch]$Compress
    )

    Set-Variable secondTo100Nanoseconds -Option Constant -Value 10000000
    Set-Variable refTimerSamplesPerSecond -Option Constant -Value 1000
    Set-Variable refCounterInterval -Option Constant -Value 65536

    switch ($SamplingRate) {
        { $_ -eq 'Low' } {
            $samplesPerSecond = 100
        }
        { $_ -eq 'Medium' } {
            $samplesPerSecond = $refTimerSamplesPerSecond
        }
        { $_ -eq 'High' } {
            $samplesPerSecond = 4000
        }
        default {
            $samplesPerSecond = $refTimerSamplesPerSecond
        }
    }
    $timerInterval = [math]::Round(1 / $samplesPerSecond * $secondTo100Nanoseconds)
    $counterInterval = [math]::Round($refTimerSamplesPerSecond / $samplesPerSecond * $refCounterInterval)

    $currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
    $isElevated = $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

    $kernelEvents = @(
        'PROC_THREAD',
        'LOADER',
        'PROFILE',
        'PERF_COUNTER'
    )

    $pmcArgs = @()
    if ($PmcProfile.Count -gt 0) {
        $kernelEvents += 'PMC_PROFILE'
        $pmcArgs += '-PmcProfile'
        $pmcArgs += $PmcProfile -join ','
        foreach ($counterName in $PmcProfile) {
            $pmcArgs += '-SetProfInt'
            $pmcArgs += $counterName
            $pmcArgs += $counterInterval
        }
    }
    if ($PmcCount.Count -gt 0) {
        $kernelEvents += 'CSWITCH'
        $pmcArgs += '-Pmc'
        $pmcArgs += $PmcCount -join ','
        $pmcArgs += 'CSWITCH'
        $pmcArgs += 'strict'
    }

    $kernelEventsArg = $kernelEvents -join '+'

    $snailProviderGuid = [System.Guid]'{460B83B6-FC11-481B-B7AA-4038CA4C4C48}'

    $xperfStartArgs = @(
        '-start', 'SnailProfiling',
        '-on', $snailProviderGuid.ToString(),
        '-f', "`"${OutputName}_user.etl`""
        '-start', '"NT Kernel Logger"',
        '-on', $kernelEventsArg,
        '-SetProfInt', 'Timer', $timerInterval,
        '-stackwalk', 'Profile',
        '-stackcaching', 1024, 4,
        '-f', "`"${OutputName}_kernel.etl`""
    )
    $xperfStartArgs += $pmcArgs

    # Write-Host $xperfStartArgs

    $commandExecutable = $Command[0]
    $commandArguments = $Command | Select-Object -Skip 1

    $eventProvider = New-Object -TypeName System.Diagnostics.Eventing.EventProvider -ArgumentList ($snailProviderGuid)
    try {
        $eventDescriptor = New-Object -TypeName System.Diagnostics.Eventing.EventDescriptor -ArgumentList (0x1, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0);

        if ($isElevated) {
            $xperfProcess = Start-Process -FilePath xperf -ArgumentList $xperfStartArgs -PassThru -Wait -NoNewWindow
        }
        else {
            $xperfProcess = Start-Process -FilePath xperf -ArgumentList $xperfStartArgs -PassThru -Wait -Verb RunAs
        }
        if ($xperfProcess.ExitCode -ne 0) { return; }
        
        try {
            $additionalArgs = @{}
            if ($commandArguments.Length -gt 0) { $additionalArgs["ArgumentList"] = $commandArguments }
            $process = Start-Process -FilePath $commandExecutable @additionalArgs -PassThru -NoNewWindow
            $eventProvider.WriteEvent([ref] $eventDescriptor, $process.Id) | Out-Null
            $process.WaitForExit()
            $process.Dispose()
        }
        finally {
            xperf -stop SnailProfiling

            if ($isElevated) {
                Start-Process -FilePath xperf -ArgumentList '-stop' -Wait -NoNewWindow
            }
            else {
                Start-Process -FilePath xperf -ArgumentList '-stop' -Wait -Verb RunAs
            }
        }

        $additionalArgs = @()
        if ($Compress) { $additionalArgs += '-compress' }

        xperf -merge "${OutputName}_user.etl" "${OutputName}_kernel.etl" "${OutputName}.etl" @additionalArgs #-suppresspii
        if ($LASTEXITCODE -eq 0) {
            Remove-Item "${OutputName}_user.etl", "${OutputName}_kernel.etl"
        }
    }
    finally {
        $eventProvider.Dispose()
    }
}
