import { DatabaseService } from './database.service';
import { Logs } from '../models/logs';
import { ProcessService } from './process.service';

const fs = require('fs');

export class LogService {

    public static log(action: string, message: string = null, ...data): void {
        const logs = new Logs();

        logs.service = action;
        logs.log = message;
        if (data && data.length) {
            try {
                logs.data = JSON.stringify(data);
            } catch (e) {
                logs.data = e.message;
            }
        }

        const now = new Date();
        const log =
            '[' + now.toISOString() + '] ' +
            '[' + action.toUpperCase() + ']' +
            ' --> ' + message
        ;

        if (data && data.length) {
            console.log(log, data);
        } else {
            console.log(log);
        }

        if (!ProcessService.debug) {
            DatabaseService.insert(logs).subscribe();
        }
    }

    public static createCopy(databasePath: string): string {
        const tempPath = '/tmp/data.db';
        fs.copyFileSync(databasePath, tempPath);
        return tempPath;
    }
}
