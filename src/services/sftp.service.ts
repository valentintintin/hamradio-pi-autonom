import SftpUpload = require('sftp-upload');
import fs = require('fs');
import { Observable, Observer } from 'rxjs';
import { LogService } from './log.service';
import { SftpConfigInterface } from '../config/sftp-config.interface';
import { ProcessService } from './process.service';
import { map } from 'rxjs/operators';

export class SftpService {

    public static send(config: SftpConfigInterface, toSend: string): Observable<string> {
        return this.sendMultiple(config, [toSend]).pipe(map(sended => sended[0]));
    }

    public static sendMultiple(config: SftpConfigInterface, toSend: string[]): Observable<string[]> {
        return new Observable<string[]>((observer: Observer<string[]>) => {
            LogService.log('sftp', 'Start sending', toSend);

            const sftp = new SftpUpload({
                host: config.host,
                username: config.username,
                path: toSend,
                remoteDir: config.remotePath,
                privateKey: fs.readFileSync(config.privateKeyPath),
                passphrase: config.privateKeyPassphrase,
                dryRun: ProcessService.debug,
            });

            sftp.on('error', function (err) {
                LogService.log('sftp', 'Send KO', err);
                observer.error(err);
            })
                .on('completed', function () {
                    LogService.log('sftp', 'Send OK');

                    observer.next(toSend);
                    observer.complete();
                })
                .upload();
        });
    }
}
