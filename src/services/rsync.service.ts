import { map } from 'rxjs/operators';
import { Observable, Observer } from 'rxjs';
import { LogService } from './log.service';
import { RsyncConfigInterface } from '../config/rsync-config.interface';
import { ProcessService } from './process.service';
import { ConfigInterface } from '../config/config.interface';

const rsync = require('rsync');

export class RsyncService {

    public static send(config: RsyncConfigInterface, toSend: string, subRemoteDir: string = ''): Observable<string> {
        return RsyncService.sendMultiple(config, [toSend], subRemoteDir).pipe(map(f => f[0]));
    }

    public static sendMultiple(config: RsyncConfigInterface, toSend: string[], subRemoteDir: string = ''): Observable<string[]> {
        if (subRemoteDir && !subRemoteDir.endsWith('/')) {
            subRemoteDir += '/';
        }

        return new Observable<string[]>((observer: Observer<string[]>) => {
            LogService.log('rsync', 'Start sending', toSend);

            const cmd = new rsync().shell('ssh -i ' + config.privateKeyPath)
                .flags(ProcessService.debug ? 'n' : '')
                .progress()
                .compress()
                .update()
                .recursive()
                .source(toSend)
                .destination(config.username + '@' + config.host + ':' + config.remotePath + subRemoteDir);

            cmd.execute(
                function (err, code, cmd) {
                    if (err) {
                        LogService.log('rsync', 'Send KO', cmd + ' ' + err);
                        observer.error(err);
                    } else {
                        LogService.log('rsync', 'Send OK', cmd);
                        observer.next(toSend);
                        observer.complete();
                    }
                }, function (data) {
                    console.log(data.toString('utf8'));
                }, function (data) {
                    LogService.log('rsync', 'Send KO data', data.toString('utf8'));
                }
            )
        });
    }

    public static runSync(config: ConfigInterface): Observable<string[]> {
        return RsyncService.sendMultiple(config.rsync, [
            LogService.createCopy(config.databasePath),
            config.sensors?.csvPath,
            config.webcam?.photosPath
        ]);
    }
}
