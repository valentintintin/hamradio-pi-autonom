import { DatabaseService } from './database.service';
import { Logs } from '../models/logs';
import fs = require('fs');

export class LogService {

    public static LOG_PATH: string = process.cwd();

    public static log(action: string, message: string = null, ...data): void {
        const logs = new Logs();

        logs.service = action;
        logs.log = message;
        if (data && data.length) {
            logs.data = JSON.stringify(data);
        }

        const now = new Date();
        const log =
            '[' + now.toLocaleString() + ']' +
            '[' + action.toUpperCase() + ']' +
            ' --> ' + message
        ;

        if (data && data.length) {
            console.log(log, data);
        } else {
            console.log(log);
        }

        // todo Add send to server
        DatabaseService.insert(logs).subscribe(_ => {
            const fileLog = LogService.LOG_PATH + '/' + now.getFullYear() + '-' + now.getMonth() + '-' + now.getDate() + '.log';

            try {
                if (!fs.existsSync(fileLog)) {
                    fs.writeFileSync(fileLog, log + '\n');
                } else {
                    fs.appendFileSync(fileLog, log + '\n');
                }
            } catch (e) {
                console.error(e, fileLog);
            }
        });
    }
}
