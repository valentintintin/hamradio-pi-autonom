import { FeatureInterface } from './config.interface';

export interface VoiceConfigInterface extends FeatureInterface {
    sentence: string;
    language: string;
}
