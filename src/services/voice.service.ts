import { Observable, Observer, of } from 'rxjs';
import { catchError, switchMap, tap } from 'rxjs/operators';
import { LogService } from './log.service';
import { RadioService } from './radio.service';
import PicoSpeaker = require('pico-speaker');

PicoSpeaker.init({
    LANGUAGE: 'fr-FR'
});

export class VoiceService {

    public static alreadyInUse: boolean;

    public static sendVoice(message: string, keepRadioOn: boolean = false): Observable<void> {
        LogService.log('voice', 'Start sending message');

        if (VoiceService.alreadyInUse) {
            LogService.log('voice', 'Already in use');
            return of(null);
        }

        VoiceService.alreadyInUse = true;

        return RadioService.pttOn().pipe(
            switchMap(_ => new Observable<void>((observer: Observer<void>) => {
                LogService.log('voice', 'Send message', message);
                PicoSpeaker.speak(message).then(_ => {
                    observer.next();
                    observer.complete();
                }).catch(err => observer.error(err));
            })),
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
}
