import { DatabaseService } from './database.service';
import { Logs } from '../models/logs';
import { ProcessService } from './process.service';
import { SftpConfigInterface } from '../config/sftp-config.interface';
import { Observable, of } from 'rxjs';
import { SftpService } from './sftp.service';
import { catchError, map } from 'rxjs/operators';
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
            DatabaseService.insert(logs).subscribe(_ => {
                const fileLog = LogService.LOG_PATH + '/' + now.getFullYear() + '-' + now.getMonth().toString(10).padStart(2, '0') + '-' + now.getDate().toString(10).padStart(2, '0') + '.log';

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

    public static send(config: SftpConfigInterface, logsPath: string): Observable<void> {
        return SftpService.send(config, logsPath).pipe(
            map(_ => null),
            catchError(e => of(null))
        );
    }
}
