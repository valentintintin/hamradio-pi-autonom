import { FeatureIntervalInterface } from './config.interface';

export interface AprsConfigInterface extends FeatureIntervalInterface {
    lat: number;
    lng: number;
    altitude: number;
    callSrc: string;
    callDest: string;
    path?: string;
    symbolTable?: string;
    symbolCode?: string;
    comment?: string;
    waitDtmfInterval?: number;
}
