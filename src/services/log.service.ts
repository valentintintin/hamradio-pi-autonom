import { DatabaseService } from './database.service';
import { Logs } from '../models/logs';
import { ProcessService } from './process.service';
import { catchError, switchMap, tap } from 'rxjs/operators';
import { of } from 'rxjs';
import fs = require('fs');

export class LogService {

    public static log(action: string, message: string = null, ...data): void {
        const logs = LogService.consoleLog(action, message, data);

        if (!ProcessService.debug) {
            DatabaseService.insert(logs).subscribe();
        }
    }

    public static consoleLog(action: string, message: string = null, ...data): Logs {
        const logs = new Logs();

        logs.service = action;
        logs.log = message;

        data = data[0];
        data = data.length > 1 ? data : data[0];

        if (data) {
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

        if (data) {
            console.log(log, data);
        } else {
            console.log(log);
        }

        return logs;
    }

    public static removeTooOld(numberDays: number = 7, numberToKeepMax: number = 50000): void {
        LogService.log('log', 'Remove old');
        const keepDate = new Date();
        keepDate.setDate(keepDate.getDate() - numberDays);
        DatabaseService.execute(`DELETE FROM ${Logs.name} WHERE id NOT IN (SELECT id FROM ${Logs.name} WHERE createdAt >= ${keepDate.getTime()} ORDER BY id DESC LIMIT ${numberToKeepMax})`).pipe(
            switchMap(_ => DatabaseService.vacuum()),
            tap(_ => {
                LogService.log('log', 'Remove old OK');
            }),
            catchError(err => {
                LogService.log('log', 'Remove old KO', err);
                return of(null);
            })
        ).subscribe();
    }

    public static createCopy(databasePath: string): string {
        const tempPath = '/tmp/data.db';
        fs.copyFileSync(databasePath, tempPath);
        return tempPath;
    }
}
