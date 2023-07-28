import { assert } from 'chai';
import * as path from 'path';
import * as snail from '../src/protocol';
import * as fixture from './server';

describe("InnerPerfData", () => {

    let documentId: number | undefined = undefined;

    before(async function () {
        if (process.env.SNAIL_ROOT_DIR === undefined) {
            assert.fail("Missing environment variable \"SNAIL_ROOT_DIR\".");
        }

        const dataDir = path.join(process.env.SNAIL_ROOT_DIR, 'tests', 'apps', 'inner', 'dist', 'linux', 'deb');

        await fixture.connection.sendNotification(snail.setDwarfSymbolFindOptionsNotificationType, {
            searchDirs: [
                path.join(dataDir, 'bin')
            ],
            noDefaultUrls: true,
            debuginfodUrls: [],
            debuginfodCacheDir: "should-not-exist-2f4f23da-ef58-4f0c-8f99-a83aed90fbbe"
        });

        const response = await fixture.connection.sendRequest(snail.readDocumentRequestType, {
            filePath: path.join(dataDir, 'record', 'inner-perf.data')
        });

        assert.isAtLeast(response.documentId, 0);

        documentId = response.documentId;
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

        assert.strictEqual(response.systemInfo.hostname, "DESKTOP-0RH3TEF");
        assert.strictEqual(response.systemInfo.platform, "5.15.90.1-microsoft-standard-WSL2");
        assert.strictEqual(response.systemInfo.architecture, "x86_64");
        assert.strictEqual(response.systemInfo.cpuName, "Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz");
        assert.strictEqual(response.systemInfo.numberOfProcessors, 8);
    });

    it("systemInfo", async () => {
        const response = await fixture.connection.sendRequest(snail.retrieveSessionInfoRequestType, {
            documentId: documentId
        });

        assert.strictEqual(response.sessionInfo.commandLine, "/usr/bin/perf record -g -o inner-perf.data /tmp/build/inner/Debug/build/inner");
        // assert.strictEqual(response.sessionInfo.date, "2023-07-02T19:47+0000");
        assert.strictEqual(response.sessionInfo.runtime, 387398400);
        assert.strictEqual(response.sessionInfo.numberOfProcesses, 1);
        assert.strictEqual(response.sessionInfo.numberOfThreads, 2);
        assert.strictEqual(response.sessionInfo.numberOfSamples, 1524);
        assert.strictEqual(response.sessionInfo.averageSamplingRate, 3933.9346780988258);
    });

    it("processes", async () => {
        const response = await fixture.connection.sendRequest(snail.retrieveProcessesRequestType, {
            documentId: documentId
        });

        assert.strictEqual(response.processes.length, 1);

        assert.strictEqual(response.processes[0].id, 248);
        assert.strictEqual(response.processes[0].name, "inner");
        assert.strictEqual(response.processes[0].startTime, 0);
        assert.strictEqual(response.processes[0].endTime, 387398400);

        assert.strictEqual(response.processes[0].threads.length, 2);

        assert.strictEqual(response.processes[0].threads[0].id, 248);
        assert.strictEqual(response.processes[0].threads[0].name, "perf-exec");
        assert.strictEqual(response.processes[0].threads[0].startTime, 0);
        assert.strictEqual(response.processes[0].threads[0].endTime, 387398400);

        assert.strictEqual(response.processes[0].threads[1].id, 248);
        assert.strictEqual(response.processes[0].threads[1].name, "inner");
        assert.strictEqual(response.processes[0].threads[1].startTime, 0);
        assert.strictEqual(response.processes[0].threads[1].endTime, 387398400);
    });

    it("hottestFunctions_1", async () => {
        const response = await fixture.connection.sendRequest(snail.retrieveHottestFunctionsRequestType, {
            documentId: documentId,
            count: 1
        });

        assert.strictEqual(response.functions.length, 1);
        assert.strictEqual(response.functions[0].processId, 248);
        assert.strictEqual(response.functions[0].function.name, "double std::generate_canonical<double, 53ul, std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>>(std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>&)");
        assert.isAtLeast(response.functions[0].function.id, 0);
        assert.strictEqual(response.functions[0].function.module, "/tmp/build/inner/Debug/build/inner");
        assert.strictEqual(response.functions[0].function.type, "function");
        assert.strictEqual(response.functions[0].function.totalSamples, 1099);
        assert.strictEqual(response.functions[0].function.selfSamples, 322);
        assert.strictEqual(response.functions[0].function.totalPercent, 72.11286089238845);
        assert.strictEqual(response.functions[0].function.selfPercent, 21.128608923884514);
    });

    it("hottestFunctions_3", async () => {
        const response = await fixture.connection.sendRequest(snail.retrieveHottestFunctionsRequestType, {
            documentId: documentId,
            count: 3
        });

        assert.strictEqual(response.functions.length, 3);

        assert.strictEqual(response.functions[0].processId, 248);
        assert.strictEqual(response.functions[0].function.name, "double std::generate_canonical<double, 53ul, std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>>(std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>&)");
        assert.isAtLeast(response.functions[0].function.id, 0);
        assert.strictEqual(response.functions[0].function.module, "/tmp/build/inner/Debug/build/inner");
        assert.strictEqual(response.functions[0].function.type, "function");
        assert.strictEqual(response.functions[0].function.totalSamples, 1099);
        assert.strictEqual(response.functions[0].function.selfSamples, 322);
        assert.strictEqual(response.functions[0].function.totalPercent, 72.11286089238845);
        assert.strictEqual(response.functions[0].function.selfPercent, 21.128608923884514);

        assert.strictEqual(response.functions[1].processId, 248);
        assert.strictEqual(response.functions[1].function.name, "libm.so.6!0x00007f6062b2d34f");
        assert.isAtLeast(response.functions[1].function.id, 0);
        assert.strictEqual(response.functions[1].function.module, "/usr/lib64/libm.so.6");
        assert.strictEqual(response.functions[1].function.type, "function");
        assert.strictEqual(response.functions[1].function.totalSamples, 321);
        assert.strictEqual(response.functions[1].function.selfSamples, 321);
        assert.strictEqual(response.functions[1].function.totalPercent, 21.062992125984252);
        assert.strictEqual(response.functions[1].function.selfPercent, 21.062992125984252);

        assert.strictEqual(response.functions[2].processId, 248);
        assert.strictEqual(response.functions[2].function.name, "std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>::_M_gen_rand()");
        assert.isAtLeast(response.functions[2].function.id, 0);
        assert.strictEqual(response.functions[2].function.module, "/tmp/build/inner/Debug/build/inner");
        assert.strictEqual(response.functions[2].function.type, "function");
        assert.strictEqual(response.functions[2].function.totalSamples, 156);
        assert.strictEqual(response.functions[2].function.selfSamples, 156);
        assert.strictEqual(response.functions[2].function.totalPercent, 10.236220472440944);
        assert.strictEqual(response.functions[2].function.selfPercent, 10.236220472440944);
    });


    it("callTreeHotPath", async () => {
        const response = await fixture.connection.sendRequest(snail.retrieveCallTreeHotPathRequestType, {
            documentId: documentId,
            processId: 248
        });

        const root_id = 4294967295; // uint32 max

        assert.strictEqual(response.root.id, root_id);
        assert.strictEqual(response.root.functionId, root_id);
        assert.isTrue(response.root.isHot);
        assert.strictEqual(response.root.module, "[multiple]");
        assert.strictEqual(response.root.name, "inner (PID: 248)");
        assert.strictEqual(response.root.totalSamples, 1524);
        assert.strictEqual(response.root.selfSamples, 0);
        assert.strictEqual(response.root.totalPercent, 100);
        assert.strictEqual(response.root.selfPercent, 0);
        assert.strictEqual(response.root.type, "process");
        assert.strictEqual(response.root.children?.length, 3);

        assert.strictEqual(response.root.children?.at(0)?.name, "ld-linux-x86-64.so.2!0x00007f6062e6e3b0");
        assert.isFalse(response.root.children?.at(0)?.isHot);
        assert.isNull(response.root.children?.at(0)?.children);

        assert.strictEqual(response.root.children?.at(1)?.name, "ld-linux-x86-64.so.2!0x00007f6062e6e3b8");
        assert.isFalse(response.root.children?.at(1)?.isHot);
        assert.isNull(response.root.children?.at(1)?.children);

        let next = response.root.children?.at(2);
        assert.strictEqual(next?.name, "inner!0x0000000000402255");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 1);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "libc.so.6!0x00007f6062938c0b");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 1);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "libc.so.6!0x00007f6062938b4a");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 2);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "main");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 3);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "make_random_vector(std::vector<double, std::allocator<double>>&, unsigned long)");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 5);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "double std::uniform_real_distribution<double>::operator()<std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>>(std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>&)");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 5);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "double std::uniform_real_distribution<double>::operator()<std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>>(std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>&, std::uniform_real_distribution<double>::param_type const&)");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 5);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "std::__detail::_Adaptor<std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>, double>::operator()()");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 4);

        next = next?.children?.at(0);
        assert.strictEqual(next?.name, "double std::generate_canonical<double, 53ul, std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>>(std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>&)");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 20);

        next = next?.children?.at(3);
        assert.strictEqual(next?.name, "libm.so.6!0x00007f6062b2d34f");
        assert.isTrue(next?.isHot);
        assert.strictEqual(next?.children?.length, 0);
    });

    it("functionPage", async () => {
        const response = await fixture.connection.sendRequest(snail.retrieveFunctionsPageRequestType, {
            documentId: documentId,
            processId: 248,
            pageSize: 3,
            pageIndex: 0
        });

        assert.strictEqual(response.functions.length, 3);

        assert.isAtLeast(response.functions[0].id, 0);
        assert.strictEqual(response.functions[0].module, "/tmp/build/inner/Debug/build/inner");
        assert.strictEqual(response.functions[0].name, "double std::generate_canonical<double, 53ul, std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>>(std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>&)");
        assert.strictEqual(response.functions[0].type, "function");
        assert.strictEqual(response.functions[0].totalSamples, 1099);
        assert.strictEqual(response.functions[0].selfSamples, 322);
        assert.strictEqual(response.functions[0].totalPercent, 72.11286089238845);
        assert.strictEqual(response.functions[0].selfPercent, 21.128608923884514);

        assert.isAtLeast(response.functions[1].id, 0);
        assert.strictEqual(response.functions[1].module, "/usr/lib64/libm.so.6");
        assert.strictEqual(response.functions[1].name, "libm.so.6!0x00007f6062b2d34f");
        assert.strictEqual(response.functions[1].totalSamples, 321);
        assert.strictEqual(response.functions[1].selfSamples, 321);
        assert.strictEqual(response.functions[1].totalPercent, 21.062992125984252);
        assert.strictEqual(response.functions[1].selfPercent, 21.062992125984252);
        assert.strictEqual(response.functions[1].type, "function");

        assert.isAtLeast(response.functions[2].id, 0);
        assert.strictEqual(response.functions[2].module, "/tmp/build/inner/Debug/build/inner");
        assert.strictEqual(response.functions[2].name, "std::mersenne_twister_engine<unsigned long, 32ul, 624ul, 397ul, 31ul, 2567483615ul, 11ul, 4294967295ul, 7ul, 2636928640ul, 15ul, 4022730752ul, 18ul, 1812433253ul>::_M_gen_rand()");
        assert.strictEqual(response.functions[2].totalSamples, 156);
        assert.strictEqual(response.functions[2].selfSamples, 156);
        assert.strictEqual(response.functions[2].totalPercent, 10.236220472440944);
        assert.strictEqual(response.functions[2].selfPercent, 10.236220472440944);
        assert.strictEqual(response.functions[2].type, "function");
    });

    it("expandCallTreeNode", async () => {

        const hot_path = await fixture.connection.sendRequest(snail.retrieveCallTreeHotPathRequestType, {
            documentId: documentId,
            processId: 248
        });

        var current: snail.CallTreeNode | null = hot_path.root;
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
            documentId: documentId,
            processId: 248,
            nodeId: current?.id
        });

        assert.strictEqual(response.children.length, 1);

        assert.isAtLeast(response.children[0].id, 0);
        assert.isAtLeast(response.children[0].functionId, 0);
        assert.isFalse(response.children[0].isHot);
        assert.strictEqual(response.children[0].module, "/tmp/build/inner/Debug/build/inner");
        assert.strictEqual(response.children[0].name, "std::vector<double, std::allocator<double>>::operator[](unsigned long) const");
        assert.strictEqual(response.children[0].totalSamples, 6);
        assert.strictEqual(response.children[0].selfSamples, 6);
        assert.strictEqual(response.children[0].totalPercent, 0.3937007874015748);
        assert.strictEqual(response.children[0].selfPercent, 0.3937007874015748);
        assert.strictEqual(response.children[0].type, "function");
        assert.strictEqual(response.children[0].children?.length, 0);
    });

    it("callersCalleesMain", async () => {

        const functionsPage = await fixture.connection.sendRequest(snail.retrieveFunctionsPageRequestType, {
            documentId: documentId,
            processId: 248,
            pageSize: 200,
            pageIndex: 0
        });

        const func = functionsPage.functions.find(func => func.name == "main");
        assert.isDefined(func);

        const response = await fixture.connection.sendRequest(snail.retrieveCallersCalleesRequestType, {
            documentId: documentId,
            processId: 248,
            functionId: func!.id,
            maxEntries: 2
        });

        assert.strictEqual(response.function.id, func!.id);
        assert.strictEqual(response.callees.length, 2);
        assert.strictEqual(response.callees[0].name, "make_random_vector(std::vector<double, std::allocator<double>>&, unsigned long)");
        assert.strictEqual(response.callees[1].name, "[others]");

        assert.strictEqual(response.callers.length, 1);
        assert.strictEqual(response.callers[0].name, "libc.so.6!0x00007f6062938b4a");
    });

    it("lineInfoMain", async () => {

        const functionsPage = await fixture.connection.sendRequest(snail.retrieveFunctionsPageRequestType, {
            documentId: documentId,
            processId: 248,
            pageSize: 200,
            pageIndex: 0
        });

        const func = functionsPage.functions.find(func => func.name == "main");
        assert.isDefined(func);

        const response = await fixture.connection.sendRequest(snail.retrieveLineInfoRequestType, {
            documentId: documentId,
            processId: 248,
            functionId: func!.id,
        });

        assert.strictEqual(response!.filePath.replace('\\', '/'), "/tmp/snail-server/tests/apps/inner/main.cpp");
        assert.strictEqual(response!.lineNumber, 57);
        assert.strictEqual(response!.totalSamples, 1502);
        assert.strictEqual(response!.selfSamples, 0);
        assert.strictEqual(response!.totalPercent, 98.55643044619423);
        assert.strictEqual(response!.selfPercent, 0);

        response!.lineHits.sort((a, b) => {
            return a.lineNumber - b.lineNumber;
        });

        assert.strictEqual(response!.lineHits.length, 2);

        assert.strictEqual(response!.lineHits[0].lineNumber, 69);
        assert.strictEqual(response!.lineHits[0].totalSamples, 759);
        assert.strictEqual(response!.lineHits[0].selfSamples, 0);
        assert.strictEqual(response!.lineHits[0].totalPercent, 49.803149606299215);
        assert.strictEqual(response!.lineHits[0].selfPercent, 0);

        assert.strictEqual(response!.lineHits[1].lineNumber, 71);
        assert.strictEqual(response!.lineHits[1].totalSamples, 743);
        assert.strictEqual(response!.lineHits[1].selfSamples, 0);
        assert.strictEqual(response!.lineHits[1].totalPercent, 48.75328083989501);
        assert.strictEqual(response!.lineHits[1].selfPercent, 0);
    });
});
