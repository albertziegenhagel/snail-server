param(
    [String]$XPerfPath,
    [String]$AppPath,
    [String]$OutputName
)

xperf -SetProfInt 2000 -on PROC_THREAD+LOADER+PMC_PROFILE+PROFILE -PmcProfile BranchMispredictions, LLCReference, LLCMisses -stackwalk Profile -PidNewProcess $AppPath -WaitForNewProcess -f "${OutputName}.etl"
xperf -stop
xperf -merge "${OutputName}.etl" "${OutputName}_merged.etl"
