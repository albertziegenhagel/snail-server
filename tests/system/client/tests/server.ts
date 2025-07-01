
import { assert } from 'chai';
import * as cp from 'child_process';
import * as fs from 'fs';
import * as rpc from 'vscode-jsonrpc/node';
import * as snail from '../src/protocol';

export let serverProcess: cp.ChildProcess;
export let connection: rpc.MessageConnection;

export const mochaGlobalSetup = async function () {
    if (process.env.SNAIL_SERVER_EXE === undefined) {
        assert.fail("Missing environment variable \"SNAIL_SERVER_EXE\".");
    }

    let pipeName = rpc.generateRandomPipeName();

    const serverPath = process.env.SNAIL_SERVER_EXE;
    const serverArgs: string[] = [];
    serverArgs.push("--pipe");
    serverArgs.push(pipeName);
    // serverArgs.push("--debug");

    const pipeTransport = await rpc.createClientPipeTransport(pipeName);
    serverProcess = cp.spawn(serverPath, serverArgs);
    let serverLog = fs.createWriteStream('./server-log.log', { flags: 'a' });

    if (!serverProcess || !serverProcess.pid) {
        assert.fail("Failed to start server process.");
    }

    serverProcess.stdout?.pipe(serverLog);
    serverProcess.stderr?.pipe(serverLog);

    const [reader, writer] = await pipeTransport.onConnected();

    connection = rpc.createMessageConnection(reader, writer);

    connection.listen();

    // connection.trace(rpc.Trace.Verbose, {
    //     log: (message: string | any, data?: string) => {
    //         console.log(message, data)
    //     }
    // });

    const response = await connection.sendRequest(snail.initializeRequestType, {});

    assert.isTrue(response.success);
};

export const mochaGlobalTeardown = async function () {

    await connection.sendRequest(snail.shutdownRequestType, null);
    await connection.sendNotification(snail.exitNotificationType);

    connection.end();
    connection.dispose();
};
