import { Database } from 'sqlite3';
import { LogService } from './log.service';
import { Observable, Observer } from 'rxjs';
import { Entity } from '../models/entity';
import { Logs } from '../models/logs';

const sqlite3 = require('sqlite3').verbose();

export class DatabaseService {

    private static db: Database;

    public static openDatabase(pathDatabase: string): Observable<void> {
        return new Observable<void>((observer: Observer<void>) => {
            try {
                DatabaseService.db = new sqlite3.Database(pathDatabase + '/data.db', err => {
                    if (err) {
                        LogService.log('database', 'Open KO', err);
                        observer.error(err);
                    }
                    observer.next(null);
                    observer.complete();
                });
            } catch (e) {
                LogService.log('database', 'Open KO', e);
                observer.error(e);
            }
        });
    }

    public static close(): Observable<void> {
        return new Observable<void>((observer: Observer<void>) => {
            DatabaseService.db.close(err => {
                if (err) {
                    LogService.log('database', 'Close KO', err);
                    observer.error(err);
                }
                observer.next(null);
                observer.complete();
            });
        });
    }

    public static insert(data: Entity, ignoreErrors: boolean = true): Observable<Entity> {
        return new Observable<Entity>((observer: Observer<Entity>) => {
            const query = `INSERT INTO ${data.constructor.name}(${Object.keys(data).join(',')}) VALUES(${Object.keys(data).map(k => `'${data[k]}'`).join(',')})`;
            try {
                DatabaseService.db.exec(query, err => {
                    if (err) {
                        if (!(data instanceof Logs)) {
                            LogService.log('database', 'Insert KO', err, query);
                        }
                        if (!ignoreErrors) {
                            observer.error(err);
                        }
                    }
                    observer.next(data);
                    observer.complete();
                });
            } catch (e) {
                if (!(data instanceof Logs)) {
                    LogService.log('database', 'Insert', query);
                }
                if (!ignoreErrors) {
                    observer.error(e);
                } else {
                    observer.next(data);
                    observer.complete();
                }
            }
        });
    }
}
