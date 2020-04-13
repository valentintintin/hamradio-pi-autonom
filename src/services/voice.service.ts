import { Observable, Observer } from 'rxjs';
import { catchError, switchMap } from 'rxjs/operators';
import { LogService } from './log.service';
import { RadioService } from './radio.service';
import PicoSpeaker = require('pico-speaker');

PicoSpeaker.init({
    LANGUAGE: 'fr-FR'
});

export class VoiceService {

    public static sendVoice(message: string, keepRadioOn: boolean = false): Observable<void> {
        LogService.log('voice', 'Start sending message');

        return RadioService.pttOn().pipe(
            switchMap(_ => new Observable<void>((observer: Observer<void>) => {
                LogService.log('voice', 'Send message', message);
                PicoSpeaker.speak(message).then(_ => {
                    observer.next();
                    observer.complete();
                }).catch(err => observer.error(err));
            })),
            switchMap(_ => RadioService.pttOff(!keepRadioOn)),
            catchError(err => {
                LogService.log('voice', 'Send message KO', err);
                return RadioService.pttOff(!keepRadioOn);
            }),
        );
    }
}
