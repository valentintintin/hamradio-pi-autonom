import { Observable, Observer, of } from 'rxjs';
import { catchError, map, switchMap, tap } from 'rxjs/operators';
import { LogService } from './log.service';
import { RadioService } from './radio.service';
import { VoiceConfigInterface } from '../config/voice-config.interface';
import { AudioService } from './audio.service';
import ChildProcess = require('child_process');

export class VoiceService {

    public static alreadyInUse: boolean;

    public static sendVoice(message: string, keepRadioOn: boolean = false, config: VoiceConfigInterface = null): Observable<void> {
        LogService.log('voice', 'Start sending message');

        if (VoiceService.alreadyInUse) {
            LogService.log('voice', 'Already in use');
            return of(null);
        }

        VoiceService.alreadyInUse = true;


        LogService.log('voice', 'Send message', message)
        return VoiceService.generate(message, config.language).pipe(
            switchMap(path => RadioService.pttOn().pipe(map(_ => path))),
            switchMap(path => AudioService.play(path, 3)),
            switchMap(_ => RadioService.pttOff(!keepRadioOn)),
            tap(_ => {
                VoiceService.alreadyInUse = false;
                LogService.log('voice', 'Send message OK');
            }),
            catchError(err => {
                VoiceService.alreadyInUse = false;
                LogService.log('voice', 'Send message KO', err);
                return RadioService.pttOff(!keepRadioOn);
            }),
        );
    }

    private static generate(message: string, language: string): Observable<string> {
        return new Observable<string>((observer: Observer<string>) => {
            const pathWav = '/tmp/voice.wav';
            try {
                ChildProcess.execSync(`pico2wave -l ${language} -w ${pathWav} "${message}"`, {
                    encoding: 'utf8',
                    stdio: 'pipe'
                });
                observer.next(pathWav);
                observer.complete();
            } catch (e) {
                observer.error(e);
            }
        });
    }
}
