import { Observable, Observer } from 'rxjs';
import { LogService } from './log.service';
import ChildProcess = require('child_process');

export class AudioService {

    public static record(seconds: number = 10): Observable<string> {
        return new Observable<string>((observer: Observer<string>) => {
            LogService.log('record', 'Start recording audio', seconds);
            const pathWav = '/tmp/record.wav';
            try {
                ChildProcess.execSync(`rec -c 1 ${pathWav} trim 0 ${seconds}`, {
                    encoding: 'utf8',
                    stdio: 'pipe'
                });
                observer.next(pathWav);
                LogService.log('record', 'Recording audio OK');
                observer.complete();
            } catch (e) {
                observer.error(e);
            }
        });
    }

    public static play(file: string): Observable<void> {
        return new Observable<void>((observer: Observer<void>) => {
            LogService.log('play', 'Start playing file', file);
            try {
                ChildProcess.execSync(`play ${file}`, {
                    encoding: 'utf8',
                    stdio: 'pipe'
                });
                observer.next();
                LogService.log('play', 'Playing file OK');
                observer.complete();
            } catch (e) {
                observer.error(e);
            }
        });
    }
}
