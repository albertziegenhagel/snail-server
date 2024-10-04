import { assert } from 'chai';
import * as path from 'path';
import * as fs from 'fs-extra';
import * as snail from '../src/protocol';
import * as fixture from './server';

describe("InnerDiagsession", function () {
    this.timeout(60 * 1000);

    let documentId: number | undefined = undefined;
    let sourceId: number | undefined = undefined;

    let tempPdbFile: string;

    before(async function () {
        if (process.env.SNAIL_ROOT_DIR === undefined) {
            assert.fail("Missing environment variable \"SNAIL_ROOT_DIR\".");
        }
        if (process.env.SNAIL_TEMP_DIR === undefined) {
            assert.fail("Missing environment variable \"SNAIL_TEMP_DIR\".");
        }

        const dataDir = path.join(process.env.SNAIL_ROOT_DIR, 'tests', 'apps', 'inner', 'dist', 'windows', 'deb');
        const cacheDir = path.join(process.env.SNAIL_TEMP_DIR, "cache")

        // Copy the file to a different path, under a different name so that we can test the module path map
        tempPdbFile = path.join(process.env.SNAIL_TEMP_DIR, 'inner-renamed.pdb')
        fs.copySync(path.join(dataDir, 'bin', 'inner.pdb'), tempPdbFile);

        await fixture.connection.sendNotification(snail.setModulePathMapsNotificationType, {
            simpleMaps: [[
                "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.pdb",
                "inner-renamed.pdb"
            ]]
        });

        await fixture.connection.sendNotification(snail.setModuleFiltersNotificationType, {
            mode: snail.ModuleFilterMode.onlyIncluded,
            include: [
                "*inner.exe",
                "*ntdll.dll"
            ],
            exclude: []
        });

        await fixture.connection.sendNotification(snail.setPdbSymbolFindOptionsNotificationType, {
            searchDirs: [
                process.env.SNAIL_TEMP_DIR
            ],
            noDefaultUrls: true,
            symbolServerUrls: [
                "https://msdl.microsoft.com/download/symbols"
            ],
            symbolCacheDir: cacheDir
        });

        const response = await fixture.connection.sendRequest(snail.readDocumentRequestType, {
            filePath: path.join(dataDir, 'record', 'inner.diagsession')
        });

        fs.removeSync(cacheDir);

        assert.isAtLeast(response.documentId, 0);

        documentId = response.documentId;

        sourceId = 0; // FIXME: modify the test to retrieve this?
    });

    after(async function () {
        await fixture.connection.sendNotification(snail.closeDocumentNotificationType, {
            documentId: documentId
        });
    });

    it("systemInfo", async () => {
        const response = await fixture.connection.sendRequest(snail.retrieveSystemInfoRequestType, {
            documentId: documentId
        });

        assert.strictEqual(response.systemInfo.hostname, "fv-az448-164");
        assert.strictEqual(response.systemInfo.platform, "Windows Server 2022 Datacenter");
        assert.strictEqual(response.systemInfo.architecture, "x64");
        assert.strictEqual(response.systemInfo.cpuName, "Intel(R) Xeon(R) Platinum 8272CL CPU @ 2.60GHz");
        assert.strictEqual(response.systemInfo.numberOfProcessors, 2);
    });

    it("sampleSources", async () => {
        const response = await fixture.connection.sendRequest(snail.retrieveSampleSourcesRequestType, {
            documentId: documentId
        });

        assert.strictEqual(response.sampleSources.length, 1);

        const timerSource = response.sampleSources.find(sourceInfo => sourceInfo.name == "Timer");
        assert.isDefined(timerSource);
        assert.strictEqual(timerSource!.id, sourceId);
        assert.strictEqual(timerSource!.name, "Timer");
        assert.strictEqual(timerSource!.numberOfSamples, 292);
        assert.strictEqual(timerSource!.averageSamplingRate, 195.95397980519758);
        assert.isTrue(timerSource!.hasStacks);
    });

    it("sessionInfo", async () => {
        const response = await fixture.connection.sendRequest(snail.retrieveSessionInfoRequestType, {
            documentId: documentId
        });

        assert.strictEqual(response.sessionInfo.commandLine, "\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\Team Tools\\DiagnosticsHub\\Collector\\VSDiagnostics.exe\" start 8822D5E9-64DD-5269-B4F5-5387BD6C2FCB /launch:D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe /loadAgent:4EA90761-2248-496C-B854-3C0399A591A4;DiagnosticsHub.CpuAgent.dll ");
        assert.strictEqual(response.sessionInfo.date, "2023-07-01T12:01+0000");
        assert.strictEqual(response.sessionInfo.runtime, 3465175800);
        assert.strictEqual(response.sessionInfo.numberOfProcesses, 1);
        assert.strictEqual(response.sessionInfo.numberOfThreads, 4);
        assert.strictEqual(response.sessionInfo.numberOfSamples, 292);
    });

    it("processes", async () => {
        const response = await fixture.connection.sendRequest(snail.retrieveProcessesRequestType, {
            documentId: documentId
        });

        assert.strictEqual(response.processes.length, 1);
        assert.strictEqual(response.processes[0].osId, 4140);
        assert.strictEqual(response.processes[0].name, "inner.exe");
        assert.strictEqual(response.processes[0].startTime, 56312800);
        assert.strictEqual(response.processes[0].endTime, 2564215900);

        assert.strictEqual(response.processes[0].threads.length, 4);

        const thread_3148 = response.processes[0].threads.find(thread => thread.osId == 3148);
        assert.isDefined(thread_3148);
        assert.strictEqual(thread_3148!.osId, 3148);
        assert.strictEqual(thread_3148!.name, null);
        assert.strictEqual(thread_3148!.startTime, 1137927400);
        assert.strictEqual(thread_3148!.endTime, 2563524800);

        const thread_3828 = response.processes[0].threads.find(thread => thread.osId == 3828);
        assert.isDefined(thread_3828);
        assert.strictEqual(thread_3828!.osId, 3828);
        assert.strictEqual(thread_3828!.name, null);
        assert.strictEqual(thread_3828!.startTime, 56313900);
        assert.strictEqual(thread_3828!.endTime, 2563993900);

        const thread_4224 = response.processes[0].threads.find(thread => thread.osId == 4224);
        assert.isDefined(thread_4224);
        assert.strictEqual(thread_4224!.osId, 4224);
        assert.strictEqual(thread_4224!.name, null);
        assert.strictEqual(thread_4224!.startTime, 1138034300);
        assert.strictEqual(thread_4224!.endTime, 2563493400);

        const thread_6180 = response.processes[0].threads.find(thread => thread.osId == 6180);
        assert.isDefined(thread_6180);
        assert.strictEqual(thread_6180!.osId, 6180);
        assert.strictEqual(thread_6180!.name, null);
        assert.strictEqual(thread_6180!.startTime, 1137662800);
        assert.strictEqual(thread_6180!.endTime, 2563540200);
    });

    it("hottestFunctions_1", async () => {
        const processesResponse = await fixture.connection.sendRequest(snail.retrieveProcessesRequestType, {
            documentId: documentId
        });
        const process = processesResponse.processes.find(proc => proc.osId == 4140);
        assert.isDefined(process);

        const response = await fixture.connection.sendRequest(snail.retrieveHottestFunctionsRequestType, {
            documentId: documentId,
            count: 1,
            sourceId: sourceId
        });

        assert.strictEqual(response.functions.length, 1);
        assert.strictEqual(response.functions[0].processKey, process!.key);
        assert.strictEqual(response.functions[0].function.name, "double __cdecl std::generate_canonical<double, 53, class std::mersenne_twister_engine<unsigned int, 32, 624, 397, 31, 2567483615, 11, 4294967295, 7, 2636928640, 15, 4022730752, 18, 1812433253>>(class std::mersenne_twister_engine<unsigned int, 32, 624, 397, 31, 2567483615, 11, 4294967295, 7, 2636928640, 15, 4022730752, 18, 1812433253> &)");
        assert.isAtLeast(response.functions[0].function.id, 0);
        assert.strictEqual(response.functions[0].function.module, "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe");
        assert.strictEqual(response.functions[0].function.type, "function");
        assert.strictEqual(response.functions[0].function.hits.length, 1);
        assert.strictEqual(response.functions[0].function.hits[0].sourceId, 0);
        assert.strictEqual(response.functions[0].function.hits[0].totalSamples, 108);
        assert.strictEqual(response.functions[0].function.hits[0].selfSamples, 34);
        assert.strictEqual(response.functions[0].function.hits[0].totalPercent, 36.986301369863014);
        assert.strictEqual(response.functions[0].function.hits[0].selfPercent, 11.643835616438356);
    });

    it("hottestFunctions_3", async () => {
        const processesResponse = await fixture.connection.sendRequest(snail.retrieveProcessesRequestType, {
            documentId: documentId
        });
        const process = processesResponse.processes.find(proc => proc.osId == 4140);
        assert.isDefined(process);

        const response = await fixture.connection.sendRequest(snail.retrieveHottestFunctionsRequestType, {
            documentId: documentId,
            count: 3,
            sourceId: sourceId
        });

        assert.strictEqual(response.functions.length, 3);

        assert.strictEqual(response.functions[0].processKey, process!.key);
        assert.strictEqual(response.functions[0].function.name, "double __cdecl std::generate_canonical<double, 53, class std::mersenne_twister_engine<unsigned int, 32, 624, 397, 31, 2567483615, 11, 4294967295, 7, 2636928640, 15, 4022730752, 18, 1812433253>>(class std::mersenne_twister_engine<unsigned int, 32, 624, 397, 31, 2567483615, 11, 4294967295, 7, 2636928640, 15, 4022730752, 18, 1812433253> &)");
        assert.isAtLeast(response.functions[0].function.id, 0);
        assert.strictEqual(response.functions[0].function.module, "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe");
        assert.strictEqual(response.functions[0].function.type, "function");
        assert.strictEqual(response.functions[0].function.hits.length, 1);
        assert.strictEqual(response.functions[0].function.hits[0].sourceId, 0);
        assert.strictEqual(response.functions[0].function.hits[0].totalSamples, 108);
        assert.strictEqual(response.functions[0].function.hits[0].selfSamples, 34);
        assert.strictEqual(response.functions[0].function.hits[0].totalPercent, 36.986301369863014);
        assert.strictEqual(response.functions[0].function.hits[0].selfPercent, 11.643835616438356);

        assert.strictEqual(response.functions[1].processKey, process!.key);
        assert.strictEqual(response.functions[1].function.name, "public: unsigned int __cdecl std::mersenne_twister<unsigned int, 32, 624, 397, 31, 2567483615, 11, 7, 2636928640, 15, 4022730752, 18>::operator()(void)");
        assert.isAtLeast(response.functions[1].function.id, 0);
        assert.strictEqual(response.functions[1].function.module, "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe");
        assert.strictEqual(response.functions[1].function.type, "function");
        assert.strictEqual(response.functions[1].function.hits.length, 1);
        assert.strictEqual(response.functions[1].function.hits[0].sourceId, 0);
        assert.strictEqual(response.functions[1].function.hits[0].totalSamples, 73);
        assert.strictEqual(response.functions[1].function.hits[0].selfSamples, 30);
        assert.strictEqual(response.functions[1].function.hits[0].totalPercent, 25);
        assert.strictEqual(response.functions[1].function.hits[0].selfPercent, 10.273972602739725);

        assert.strictEqual(response.functions[2].processKey, process!.key);
        assert.strictEqual(response.functions[2].function.name, "private: void __cdecl std::vector<double, class std::allocator<double>>::_Orphan_range_locked(double *, double *) const");
        assert.isAtLeast(response.functions[2].function.id, 0);
        assert.strictEqual(response.functions[2].function.module, "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe");
        assert.strictEqual(response.functions[2].function.type, "function");
        assert.strictEqual(response.functions[2].function.hits.length, 1);
        assert.strictEqual(response.functions[2].function.hits[0].sourceId, 0);
        assert.strictEqual(response.functions[2].function.hits[0].totalSamples, 84);
        assert.strictEqual(response.functions[2].function.hits[0].selfSamples, 24);
        assert.strictEqual(response.functions[2].function.hits[0].totalPercent, 28.767123287671232);
        assert.strictEqual(response.functions[2].function.hits[0].selfPercent, 8.219178082191782);
    });

    it("callTreeHotPath", async () => {
        const processesResponse = await fixture.connection.sendRequest(snail.retrieveProcessesRequestType, {
            documentId: documentId
        });
        const process = processesResponse.processes.find(proc => proc.osId == 4140);
        assert.isDefined(process);

        const response = await fixture.connection.sendRequest(snail.retrieveCallTreeHotPathRequestType, {
            documentId: documentId,
            sourceId: sourceId,
            processKey: process!.key
        });

        const root_id = 4294967295; // uint32 max

        assert.strictEqual(response.root.id, root_id);
        assert.strictEqual(response.root.functionId, root_id);
        assert.isTrue(response.root.isHot);
        assert.strictEqual(response.root.module, "[multiple]");
        assert.strictEqual(response.root.name, "inner.exe (PID: 4140)");
        assert.strictEqual(response.root.hits.length, 1);
        assert.strictEqual(response.root.hits[0].sourceId, 0);
        assert.strictEqual(response.root.hits[0].totalSamples, 292);
        assert.strictEqual(response.root.hits[0].selfSamples, 0);
        assert.strictEqual(response.root.hits[0].totalPercent, 100);
        assert.strictEqual(response.root.hits[0].selfPercent, 0);
        assert.strictEqual(response.root.type, "process");
        assert.strictEqual(response.root.children?.length, 3);

        assert.strictEqual(response.root.children?.at(0)?.name, "LdrInitializeThunk");
        assert.isFalse(response.root.children?.at(0)?.isHot);
        assert.isNull(response.root.children?.at(0)?.children);

        assert.strictEqual(response.root.children?.at(2)?.name, "ntoskrnl.exe!0xfffff80483a21be1");
        assert.isFalse(response.root.children?.at(2)?.isHot);
        assert.isNull(response.root.children?.at(2)?.children);

        let next = response.root.children?.at(1);
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.name, "RtlUserThreadStart");
        assert.strictEqual(next?.children?.length, 1);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "kernel32.dll!0x00007ffde2e54de0");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 2);

        next = next?.children?.at(1);
        assert.strictEqual(next?.name, "mainCRTStartup");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 1);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "__scrt_common_main");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 1);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "__scrt_common_main_seh");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 2);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "invoke_main");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 1);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "main");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 4);

        next = next?.children?.at(1);
        assert.strictEqual(next?.name, "void __cdecl make_random_vector(class std::vector<double, class std::allocator<double>> &, unsigned __int64)");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 4);

        next = next?.children?.at(2);
        assert.strictEqual(next?.name, "public: void __cdecl std::vector<double, class std::allocator<double>>::push_back(double &&)");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 3);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "private: double & __cdecl std::vector<double, class std::allocator<double>>::_Emplace_one_at_back<double>(double &&)");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 2);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "private: double & __cdecl std::vector<double, class std::allocator<double>>::_Emplace_back_with_unused_capacity<double>(double &&)");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 4);

        next = next?.children?.at(1);
        assert.strictEqual(next?.name, "private: void __cdecl std::vector<double, class std::allocator<double>>::_Orphan_range(double *, double *) const");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 1);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "private: void __cdecl std::vector<double, class std::allocator<double>>::_Orphan_range_locked(double *, double *) const");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 13);

        const finalChildren = next?.children;
        assert.isDefined(finalChildren);
        assert.isNotNull(finalChildren);

        for (const child of finalChildren!) {
            assert.isFalse(child.isHot);
        }
    });

    it("functionPageSelf", async () => {
        const processesResponse = await fixture.connection.sendRequest(snail.retrieveProcessesRequestType, {
            documentId: documentId
        });
        const process = processesResponse.processes.find(proc => proc.osId == 4140);
        assert.isDefined(process);

        const response = await fixture.connection.sendRequest(snail.retrieveFunctionsPageRequestType, {
            documentId: documentId!,
            sortBy: snail.FunctionsSortBy.selfSamples,
            sortOrder: snail.SortDirection.descending,
            sortSourceId: sourceId!,
            processKey: process!.key,
            pageSize: 3,
            pageIndex: 0
        });

        assert.strictEqual(response.functions.length, 3);

        assert.isAtLeast(response.functions[0].id, 0);
        assert.strictEqual(response.functions[0].module, "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe");
        assert.strictEqual(response.functions[0].name, "double __cdecl std::generate_canonical<double, 53, class std::mersenne_twister_engine<unsigned int, 32, 624, 397, 31, 2567483615, 11, 4294967295, 7, 2636928640, 15, 4022730752, 18, 1812433253>>(class std::mersenne_twister_engine<unsigned int, 32, 624, 397, 31, 2567483615, 11, 4294967295, 7, 2636928640, 15, 4022730752, 18, 1812433253> &)");
        assert.strictEqual(response.functions[0].hits.length, 1);
        assert.strictEqual(response.functions[0].hits[0].sourceId, 0);
        assert.strictEqual(response.functions[0].hits[0].totalSamples, 108);
        assert.strictEqual(response.functions[0].hits[0].selfSamples, 34);
        assert.strictEqual(response.functions[0].hits[0].totalPercent, 36.986301369863014);
        assert.strictEqual(response.functions[0].hits[0].selfPercent, 11.643835616438356);
        assert.strictEqual(response.functions[0].type, "function");

        assert.isAtLeast(response.functions[1].id, 0);
        assert.strictEqual(response.functions[1].module, "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe");
        assert.strictEqual(response.functions[1].name, "public: unsigned int __cdecl std::mersenne_twister<unsigned int, 32, 624, 397, 31, 2567483615, 11, 7, 2636928640, 15, 4022730752, 18>::operator()(void)");
        assert.strictEqual(response.functions[1].hits.length, 1);
        assert.strictEqual(response.functions[1].hits[0].sourceId, 0);
        assert.strictEqual(response.functions[1].hits[0].totalSamples, 73);
        assert.strictEqual(response.functions[1].hits[0].selfSamples, 30);
        assert.strictEqual(response.functions[1].hits[0].totalPercent, 25);
        assert.strictEqual(response.functions[1].hits[0].selfPercent, 10.273972602739725);
        assert.strictEqual(response.functions[1].type, "function");

        assert.isAtLeast(response.functions[2].id, 0);
        assert.strictEqual(response.functions[2].module, "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe");
        assert.strictEqual(response.functions[2].name, "private: void __cdecl std::vector<double, class std::allocator<double>>::_Orphan_range_locked(double *, double *) const");
        assert.strictEqual(response.functions[2].hits.length, 1);
        assert.strictEqual(response.functions[2].hits[0].sourceId, 0);
        assert.strictEqual(response.functions[2].hits[0].totalSamples, 84);
        assert.strictEqual(response.functions[2].hits[0].selfSamples, 24);
        assert.strictEqual(response.functions[2].hits[0].totalPercent, 28.767123287671232);
        assert.strictEqual(response.functions[2].hits[0].selfPercent, 8.219178082191782);
        assert.strictEqual(response.functions[2].type, "function");
    });


    it("functionPageTotal", async () => {
        const processesResponse = await fixture.connection.sendRequest(snail.retrieveProcessesRequestType, {
            documentId: documentId
        });
        const process = processesResponse.processes.find(proc => proc.osId == 4140);
        assert.isDefined(process);

        const response = await fixture.connection.sendRequest(snail.retrieveFunctionsPageRequestType, {
            documentId: documentId!,
            sortBy: snail.FunctionsSortBy.totalSamples,
            sortOrder: snail.SortDirection.descending,
            sortSourceId: sourceId!,
            processKey: process!.key,
            pageSize: 3,
            pageIndex: 0
        });

        assert.strictEqual(response.functions.length, 3);

        assert.isAtLeast(response.functions[0].id, 0);
        assert.strictEqual(response.functions[0].module, "C:\\Windows\\System32\\kernel32.dll");
        assert.strictEqual(response.functions[0].name, "kernel32.dll!0x00007ffde2e54de0");
        assert.strictEqual(response.functions[0].hits.length, 1);
        assert.strictEqual(response.functions[0].hits[0].sourceId, 0);
        assert.strictEqual(response.functions[0].hits[0].totalSamples, 283);
        assert.strictEqual(response.functions[0].hits[0].selfSamples, 0);
        assert.strictEqual(response.functions[0].hits[0].totalPercent, 96.91780821917808);
        assert.strictEqual(response.functions[0].hits[0].selfPercent, 0.0);
        assert.strictEqual(response.functions[0].type, "function");

        assert.isAtLeast(response.functions[1].id, 0);
        assert.strictEqual(response.functions[1].module, "C:\\Windows\\System32\\ntdll.dll");
        assert.strictEqual(response.functions[1].name, "RtlUserThreadStart");
        assert.strictEqual(response.functions[1].hits.length, 1);
        assert.strictEqual(response.functions[1].hits[0].sourceId, 0);
        assert.strictEqual(response.functions[1].hits[0].totalSamples, 283);
        assert.strictEqual(response.functions[1].hits[0].selfSamples, 0);
        assert.strictEqual(response.functions[1].hits[0].totalPercent, 96.91780821917808);
        assert.strictEqual(response.functions[1].hits[0].selfPercent, 0.0);
        assert.strictEqual(response.functions[1].type, "function");

        assert.isAtLeast(response.functions[2].id, 0);
        assert.strictEqual(response.functions[2].module, "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe");
        assert.strictEqual(response.functions[2].name, "mainCRTStartup");
        assert.strictEqual(response.functions[2].hits.length, 1);
        assert.strictEqual(response.functions[2].hits[0].sourceId, 0);
        assert.strictEqual(response.functions[2].hits[0].totalSamples, 277);
        assert.strictEqual(response.functions[2].hits[0].selfSamples, 0);
        assert.strictEqual(response.functions[2].hits[0].totalPercent, 94.86301369863014);
        assert.strictEqual(response.functions[2].hits[0].selfPercent, 0.0);
        assert.strictEqual(response.functions[2].type, "function");
    });

    it("functionPageName", async () => {
        const processesResponse = await fixture.connection.sendRequest(snail.retrieveProcessesRequestType, {
            documentId: documentId
        });
        const process = processesResponse.processes.find(proc => proc.osId == 4140);
        assert.isDefined(process);

        const response = await fixture.connection.sendRequest(snail.retrieveFunctionsPageRequestType, {
            documentId: documentId!,
            sortBy: snail.FunctionsSortBy.name,
            sortOrder: snail.SortDirection.ascending,
            sortSourceId: null,
            processKey: process!.key,
            pageSize: 3,
            pageIndex: 0
        });

        assert.strictEqual(response.functions.length, 3);

        // FIXME: the following is unstable, since Windows uses the DIA SDK to resolve symbols,
        //        while on Linux we use LLVM only. This introduces very slight changes to the function names.
        // assert.isAtLeast(response.functions[0].id, 0);
        // assert.strictEqual(response.functions[0].module, "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe");
        // assert.strictEqual(response.functions[0].name, "@ILT+1020(_RTC_CheckStackVars)");
        // assert.strictEqual(response.functions[0].hits.length, 1);
        // assert.strictEqual(response.functions[0].hits[0].sourceId, 0);
        // assert.strictEqual(response.functions[0].hits[0].totalSamples, 1);
        // assert.strictEqual(response.functions[0].hits[0].selfSamples, 1);
        // assert.strictEqual(response.functions[0].hits[0].totalPercent, 0.3424657534246575);
        // assert.strictEqual(response.functions[0].hits[0].selfPercent, 0.3424657534246575);
        // assert.strictEqual(response.functions[0].type, "function");

        // assert.isAtLeast(response.functions[1].id, 0);
        // assert.strictEqual(response.functions[1].module, "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe");
        // assert.strictEqual(response.functions[1].name, "@ILT+130(??$forward@N@std@@YA$$QEANAEAN@Z)");
        // assert.strictEqual(response.functions[1].hits.length, 1);
        // assert.strictEqual(response.functions[1].hits[0].sourceId, 0);
        // assert.strictEqual(response.functions[1].hits[0].totalSamples, 1);
        // assert.strictEqual(response.functions[1].hits[0].selfSamples, 1);
        // assert.strictEqual(response.functions[1].hits[0].totalPercent, 0.3424657534246575);
        // assert.strictEqual(response.functions[1].hits[0].selfPercent, 0.3424657534246575);
        // assert.strictEqual(response.functions[1].type, "function");

        // assert.isAtLeast(response.functions[2].id, 0);
        // assert.strictEqual(response.functions[2].module, "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe");
        // assert.strictEqual(response.functions[2].name, "@ILT+225(??R?$mersenne_twister@I$0CA@$0CHA@$0BIN@$0BP@$0JJAILANP@$0L@$06$0JNCMFGIA@$0P@$0OPMGAAAA@$0BC@@std@@QEAAIXZ)");
        // assert.strictEqual(response.functions[2].hits.length, 1);
        // assert.strictEqual(response.functions[2].hits[0].sourceId, 0);
        // assert.strictEqual(response.functions[2].hits[0].totalSamples, 1);
        // assert.strictEqual(response.functions[2].hits[0].selfSamples, 1);
        // assert.strictEqual(response.functions[2].hits[0].totalPercent, 0.3424657534246575);
        // assert.strictEqual(response.functions[2].hits[0].selfPercent, 0.3424657534246575);
        // assert.strictEqual(response.functions[2].type, "function");
    });

    it("expandCallTreeNode", async () => {
        const processesResponse = await fixture.connection.sendRequest(snail.retrieveProcessesRequestType, {
            documentId: documentId
        });
        const process = processesResponse.processes.find(proc => proc.osId == 4140);
        assert.isDefined(process);

        const hotPathResponse = await fixture.connection.sendRequest(snail.retrieveCallTreeHotPathRequestType, {
            documentId: documentId,
            sourceId: sourceId,
            processKey: process!.key
        });

        var current: snail.CallTreeNode | null = hotPathResponse.root;
        while (current && current.children) {
            if (current.name === "main") {
                break;
            }
            var next = null;
            for (const child of current.children) {
                if (child.isHot) {
                    next = child;
                    break;
                }
            }
            current = next;
        }

        assert.isNotNull(current);

        for (const child of current!.children!) {
            if (child.name.includes("compute_inner_product")) {
                current = child;
                break;
            }
        }

        const response = await fixture.connection.sendRequest(snail.expandCallTreeNodeRequestType, {
            documentId: documentId!,
            processKey: process!.key,
            nodeId: current!.id
        });

        assert.strictEqual(response.children.length, 1);

        assert.isAtLeast(response.children[0].id, 0);
        assert.isAtLeast(response.children[0].functionId, 0);
        assert.isFalse(response.children[0].isHot);
        assert.strictEqual(response.children[0].module, "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe");
        assert.strictEqual(response.children[0].name, "public: double const & __cdecl std::vector<double, class std::allocator<double>>::operator[](unsigned __int64) const");
        assert.strictEqual(response.children[0].hits.length, 1);
        assert.strictEqual(response.children[0].hits[0].sourceId, 0);
        assert.strictEqual(response.children[0].hits[0].totalSamples, 4);
        assert.strictEqual(response.children[0].hits[0].selfSamples, 4);
        assert.strictEqual(response.children[0].hits[0].totalPercent, 1.36986301369863);
        assert.strictEqual(response.children[0].hits[0].selfPercent, 1.36986301369863);
        assert.strictEqual(response.children[0].type, "function");
        assert.strictEqual(response.children[0].children?.length, 0);
    });

    it("callersCalleesMain", async () => {
        const processesResponse = await fixture.connection.sendRequest(snail.retrieveProcessesRequestType, {
            documentId: documentId
        });
        const process = processesResponse.processes.find(proc => proc.osId == 4140);
        assert.isDefined(process);

        const functionsPage = await fixture.connection.sendRequest(snail.retrieveFunctionsPageRequestType, {
            documentId: documentId!,
            sortBy: snail.FunctionsSortBy.selfSamples,
            sortOrder: snail.SortDirection.descending,
            sortSourceId: sourceId!,
            processKey: process!.key,
            pageSize: 500,
            pageIndex: 0
        });

        const func = functionsPage.functions.find(func => func.name == "main");
        assert.isDefined(func);

        const response = await fixture.connection.sendRequest(snail.retrieveCallersCalleesRequestType, {
            documentId: documentId,
            sortSourceId: sourceId,
            processKey: process!.key,
            functionId: func!.id,
            maxEntries: 2
        });

        assert.strictEqual(response.function.id, func!.id);
        assert.strictEqual(response.callees.length, 2);
        assert.strictEqual(response.callees[0].name, "void __cdecl make_random_vector(class std::vector<double, class std::allocator<double>> &, unsigned __int64)");
        assert.strictEqual(response.callees[1].name, "[others]");

        assert.strictEqual(response.callers.length, 1);
        assert.strictEqual(response.callers[0].name, "invoke_main");
    });

    it("lineInfoMain", async () => {
        const processesResponse = await fixture.connection.sendRequest(snail.retrieveProcessesRequestType, {
            documentId: documentId
        });
        const process = processesResponse.processes.find(proc => proc.osId == 4140);
        assert.isDefined(process);

        const functionsPage = await fixture.connection.sendRequest(snail.retrieveFunctionsPageRequestType, {
            documentId: documentId!,
            sortBy: snail.FunctionsSortBy.selfSamples,
            sortOrder: snail.SortDirection.descending,
            sortSourceId: sourceId!,
            processKey: process!.key,
            pageSize: 500,
            pageIndex: 0
        });
        const func = functionsPage.functions.find(func => func.name == "main");
        assert.isDefined(func);

        const response = await fixture.connection.sendRequest(snail.retrieveLineInfoRequestType, {
            documentId: documentId,
            processKey: process!.key,
            functionId: func!.id,
        });

        assert.isNotNull(response);

        assert.strictEqual(response!.filePath, "D:\\a\\snail-server\\snail-server\\tests\\apps\\inner\\main.cpp");
        assert.strictEqual(response!.lineNumber, 57);
        assert.strictEqual(response!.hits.length, 1);
        assert.strictEqual(response!.hits[0].sourceId, 0);
        assert.strictEqual(response!.hits[0].totalSamples, 275);
        assert.strictEqual(response!.hits[0].selfSamples, 0);
        assert.strictEqual(response!.hits[0].totalPercent, 94.17808219178082);
        assert.strictEqual(response!.hits[0].selfPercent, 0);

        response!.lineHits.sort((a, b) => {
            return a.lineNumber - b.lineNumber;
        });

        assert.strictEqual(response!.lineHits.length, 4);

        assert.strictEqual(response!.lineHits[0].lineNumber, 63);
        assert.strictEqual(response!.lineHits[0].hits.length, 1);
        assert.strictEqual(response!.lineHits[0].hits[0].sourceId, 0);
        assert.strictEqual(response!.lineHits[0].hits[0].totalSamples, 1);
        assert.strictEqual(response!.lineHits[0].hits[0].selfSamples, 0);
        assert.strictEqual(response!.lineHits[0].hits[0].totalPercent, 0.3424657534246575);
        assert.strictEqual(response!.lineHits[0].hits[0].selfPercent, 0);

        assert.strictEqual(response!.lineHits[1].lineNumber, 69);
        assert.strictEqual(response!.lineHits[1].hits.length, 1);
        assert.strictEqual(response!.lineHits[1].hits[0].sourceId, 0);
        assert.strictEqual(response!.lineHits[1].hits[0].totalSamples, 133);
        assert.strictEqual(response!.lineHits[1].hits[0].selfSamples, 0);
        assert.strictEqual(response!.lineHits[1].hits[0].totalPercent, 45.54794520547945);
        assert.strictEqual(response!.lineHits[1].hits[0].selfPercent, 0);

        assert.strictEqual(response!.lineHits[2].lineNumber, 71);
        assert.strictEqual(response!.lineHits[2].hits.length, 1);
        assert.strictEqual(response!.lineHits[2].hits[0].sourceId, 0);
        assert.strictEqual(response!.lineHits[2].hits[0].totalSamples, 138);
        assert.strictEqual(response!.lineHits[2].hits[0].selfSamples, 0);
        assert.strictEqual(response!.lineHits[2].hits[0].totalPercent, 47.26027397260274);
        assert.strictEqual(response!.lineHits[2].hits[0].selfPercent, 0);

        assert.strictEqual(response!.lineHits[3].lineNumber, 75);
        assert.strictEqual(response!.lineHits[3].hits.length, 1);
        assert.strictEqual(response!.lineHits[3].hits[0].sourceId, 0);
        assert.strictEqual(response!.lineHits[3].hits[0].totalSamples, 3);
        assert.strictEqual(response!.lineHits[3].hits[0].selfSamples, 0);
        assert.strictEqual(response!.lineHits[3].hits[0].totalPercent, 1.0273972602739727);
        assert.strictEqual(response!.lineHits[3].hits[0].selfPercent, 0);
    });

    it("lineInfoUnknown", async () => {
        const processesResponse = await fixture.connection.sendRequest(snail.retrieveProcessesRequestType, {
            documentId: documentId
        });
        const process = processesResponse.processes.find(proc => proc.osId == 4140);
        assert.isDefined(process);

        const functionsPage = await fixture.connection.sendRequest(snail.retrieveFunctionsPageRequestType, {
            documentId: documentId!,
            sortBy: snail.FunctionsSortBy.selfSamples,
            sortOrder: snail.SortDirection.descending,
            sortSourceId: sourceId!,
            processKey: process!.key,
            pageSize: 50,
            pageIndex: 0
        });
        const func = functionsPage.functions.find(func => func.name.startsWith("ntoskrnl.exe!"));
        assert.isDefined(func);

        const response = await fixture.connection.sendRequest(snail.retrieveLineInfoRequestType, {
            documentId: documentId,
            processKey: process!.key,
            functionId: func!.id,
        });

        assert.isNull(response);
    });

    it("filterTime", async () => {
        await fixture.connection.sendRequest(snail.setSampleFiltersRequestType, {
            documentId: documentId!,
            minTime: 2200000000,
            maxTime: 2300000000,
            excludedProcesses: [],
            excludedThreads: []
        });

        const processesResponse = await fixture.connection.sendRequest(snail.retrieveProcessesRequestType, {
            documentId: documentId
        });
        const process = processesResponse.processes.find(proc => proc.osId == 4140);
        assert.isDefined(process);

        const response = await fixture.connection.sendRequest(snail.retrieveHottestFunctionsRequestType, {
            documentId: documentId,
            sourceId: sourceId,
            count: 1
        });

        assert.strictEqual(response.functions.length, 1);
        assert.strictEqual(response.functions[0].processKey, process!.key);
        assert.strictEqual(response.functions[0].function.name, "double __cdecl std::generate_canonical<double, 53, class std::mersenne_twister_engine<unsigned int, 32, 624, 397, 31, 2567483615, 11, 4294967295, 7, 2636928640, 15, 4022730752, 18, 1812433253>>(class std::mersenne_twister_engine<unsigned int, 32, 624, 397, 31, 2567483615, 11, 4294967295, 7, 2636928640, 15, 4022730752, 18, 1812433253> &)");
        assert.isAtLeast(response.functions[0].function.id, 0);
        assert.strictEqual(response.functions[0].function.module, "D:\\a\\snail-server\\snail-server\\inner\\Debug\\build\\inner.exe");
        assert.strictEqual(response.functions[0].function.type, "function");
        assert.strictEqual(response.functions[0].function.hits.length, 1);
        assert.strictEqual(response.functions[0].function.hits[0].sourceId, 0);
        assert.strictEqual(response.functions[0].function.hits[0].totalSamples, 42);
        assert.strictEqual(response.functions[0].function.hits[0].selfSamples, 15);
        assert.strictEqual(response.functions[0].function.hits[0].totalPercent, 42.42424242424242);
        assert.strictEqual(response.functions[0].function.hits[0].selfPercent, 15.151515151515152);
    });

    it("filterProcess", async () => {

        const processesResponse = await fixture.connection.sendRequest(snail.retrieveProcessesRequestType, {
            documentId: documentId
        });
        const process = processesResponse.processes.find(proc => proc.osId == 4140);
        assert.isDefined(process);

        await fixture.connection.sendRequest(snail.setSampleFiltersRequestType, {
            documentId: documentId!,
            minTime: null,
            maxTime: null,
            excludedProcesses: [process!.key],
            excludedThreads: []
        });

        const response = await fixture.connection.sendRequest(snail.retrieveHottestFunctionsRequestType, {
            documentId: documentId,
            sourceId: sourceId,
            count: 10
        });

        assert.strictEqual(response.functions.length, 0);
    });

    it("filterThreads", async () => {

        const processesResponse = await fixture.connection.sendRequest(snail.retrieveProcessesRequestType, {
            documentId: documentId
        });
        const process = processesResponse.processes.find(proc => proc.osId == 4140);
        assert.isDefined(process);

        const thread3828 = process?.threads.find(thread => thread.osId == 3828);
        assert.isDefined(thread3828);

        const thread4224 = process?.threads.find(thread => thread.osId == 4224);
        assert.isDefined(thread4224);

        await fixture.connection.sendRequest(snail.setSampleFiltersRequestType, {
            documentId: documentId!,
            minTime: null,
            maxTime: null,
            excludedProcesses: [],
            excludedThreads: [thread3828!.key, thread4224!.key]
        });

        const response = await fixture.connection.sendRequest(snail.retrieveHottestFunctionsRequestType, {
            documentId: documentId,
            sourceId: sourceId,
            count: 1
        });

        assert.strictEqual(response.functions.length, 1);
        assert.strictEqual(response.functions[0].processKey, process!.key);
        assert.strictEqual(response.functions[0].function.name, "ntoskrnl.exe!0xfffff80483a2fc56");
        assert.isAtLeast(response.functions[0].function.id, 0);
        assert.strictEqual(response.functions[0].function.module, "C:\\Windows\\system32\\ntoskrnl.exe");
        assert.strictEqual(response.functions[0].function.type, "function");
        assert.strictEqual(response.functions[0].function.hits.length, 1);
        assert.strictEqual(response.functions[0].function.hits[0].sourceId, 0);
        assert.strictEqual(response.functions[0].function.hits[0].totalSamples, 1);
        assert.strictEqual(response.functions[0].function.hits[0].selfSamples, 1);
        assert.strictEqual(response.functions[0].function.hits[0].totalPercent, 33.333333333333336);
        assert.strictEqual(response.functions[0].function.hits[0].selfPercent, 33.333333333333336);
    });
});
