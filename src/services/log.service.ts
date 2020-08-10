import { DatabaseService } from './database.service';
import { Logs } from '../models/logs';
import { ProcessService } from './process.service';
import { SftpConfigInterface } from '../config/sftp-config.interface';
import { Observable, of } from 'rxjs';
import { SftpService } from './sftp.service';
import { catchError, map } from 'rxjs/operators';

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

    public static send(config: SftpConfigInterface, databasePath: string): Observable<void> {
        return SftpService.send(config, databasePath).pipe(
            map(_ => null),
            catchError(e => of(null))
        );
    }
}
