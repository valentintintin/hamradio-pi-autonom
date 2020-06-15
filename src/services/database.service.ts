import { Database } from 'sqlite3';
import { LogService } from './log.service';
import { Observable, Observer } from 'rxjs';
import { Entity } from '../models/entity';
import { Logs } from '../models/logs';
import { map, switchMap } from 'rxjs/operators';
import { Variables } from '../models/variables';
import fs = require('fs');

const sqlite3 = require('sqlite3').verbose();

export class DatabaseService {

    private static db: Database;

    public static openDatabase(pathDatabase: string): Observable<Database> {
        let flag = sqlite3.OPEN_READWRITE;

        let req = new Observable<Database>((observer: Observer<Database>) => {
            try {
                DatabaseService.db = new sqlite3.Database(pathDatabase, flag, err => {
                    if (err) {
                        LogService.log('database', 'Open KO', err);
                        observer.error(err);
                    }
                    observer.next(DatabaseService.db);
                    observer.complete();
                });
            } catch (e) {
                LogService.log('database', 'Open KO', e);
                observer.error(e);
            }
        });

        if (!fs.existsSync(pathDatabase)) {
            flag |= sqlite3.OPEN_CREATE;
            req = req.pipe(
                switchMap(_ => {
                    const sql = fs.readFileSync(__dirname + '/../create.sql', 'utf8');
                    return new Observable<Database>((observer: Observer<Database>) => {
                        LogService.log('database', 'Creating tables');

                        DatabaseService.db.exec(sql, err => {
                            if (err) {
                                LogService.log('database', 'Create KO', err);
                                observer.error(err);
                            }
                            observer.next(DatabaseService.db);
                            observer.complete();
                        });
                    });
                }),
            )
        }

        return req;
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

    public static selectAll<T>(entity: string, limit: number = 100, order: string = 'DESC', ignoreErrors: boolean = true): Observable<T[]> {
        return new Observable<T[]>((observer: Observer<T[]>) => {
            const query = `SELECT * FROM ${entity} ORDER BY id ${order} LIMIT ${limit}`;
            try {
                DatabaseService.db.all(query, (err, rows) => {
                    if (err) {
                        LogService.log('database', 'Select KO', err, query);
                        if (!ignoreErrors) {
                            observer.error(err);
                        }
                    }
                    observer.next(rows);
                    observer.complete();
                });
            } catch (e) {
                LogService.log('database', 'Select KO', e, query);
                if (!ignoreErrors) {
                    observer.error(e);
                }
                observer.next([]);
                observer.complete();
            }
        });
    }

    public static selectLast<T>(entity: string, ignoreErrors: boolean = true): Observable<T> {
        return new Observable<T>((observer: Observer<T>) => {
            const query = `SELECT * FROM ${entity} ORDER BY id DESC LIMIT 1`;
            try {
                DatabaseService.db.get(query, (err, row) => {
                    if (err) {
                        LogService.log('database', 'Select KO', err, query);
                        if (!ignoreErrors) {
                            observer.error(err);
                        }
                    }
                    observer.next(row);
                    observer.complete();
                });
            } catch (e) {
                LogService.log('database', 'Select KO', e, query);
                if (!ignoreErrors) {
                    observer.error(e);
                }
                observer.next(null);
                observer.complete();
            }
        });
    }

    public static insert(data: Entity, ignoreErrors: boolean = true): Observable<Entity> {
        return new Observable<Entity>((observer: Observer<Entity>) => {
            const keys = Object.keys(data).filter(d => d !== 'id' && !!data[d]);
            const query = `INSERT INTO ${data.constructor.name}(${keys.join(',')}) VALUES(${keys.map(k => `'${data[k]}'`).join(',')})`;
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
                    LogService.log('database', 'Insert KO', e, query);
                }
                if (!ignoreErrors) {
                    observer.error(e);
                }
                observer.next(data);
                observer.complete();
            }
        });
    }

    public static update(data: Entity, key: string | number, ignoreErrors: boolean = true): Observable<Entity> {
        return new Observable<Entity>((observer: Observer<Entity>) => {
            const query = `UPDATE ${data.constructor.name} SET ${Object.keys(data).filter(d => d !== 'id' && !!data[d]).map(k => `${k} = '${data[k]}'`).join(',')} WHERE ${key} = '${data[key]}'`;
            try {
                DatabaseService.db.exec(query, err => {
                    if (err) {
                        if (!(data instanceof Logs)) {
                            LogService.log('database', 'Update KO', err, query);
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
                    LogService.log('database', 'Update KO', e, query);
                }
                if (!ignoreErrors) {
                    observer.error(e);
                }
                observer.next(data);
                observer.complete();
            }
        });
    }

    public static readVariable<T>(name: string, fallback: T = null, ignoreErrors: boolean = true): Observable<T> {
        return new Observable<T>((observer: Observer<T>) => {
            const query = `SELECT data FROM variables WHERE name = '${name}'`;
            try {
                DatabaseService.db.get(query, (err, row) => {
                    if (err) {
                        LogService.log('database', 'Select KO', err, query);
                        if (!ignoreErrors) {
                            observer.error(err);
                        }
                        observer.next(fallback);
                        observer.complete();
                    }
                    observer.next(row.data);
                    observer.complete();
                });
            } catch (e) {
                LogService.log('database', 'Select KO', e, query);
                if (!ignoreErrors) {
                    observer.error(e);
                }
                observer.next(fallback);
                observer.complete();
            }
        });
    }

    public static updateVariable<T>(name: string, value: T, ignoreErrors: boolean = true): Observable<T> {
        const variable = new Variables();
        variable.name = name;
        variable.data = value + '';
        return DatabaseService.update(variable, 'name', ignoreErrors).pipe(map(_ => value));
    }
}
