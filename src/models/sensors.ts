import { Entity } from './entity';

export class Sensors extends Entity {
    public voltageBattery: number;
    public voltageSolar: number;
    public currentBattery: number;
    public currentSolar: number;
    public currentCharge: number;
    public temperatureBattery: number;
    public temperatureCpu: number;
    public temperatureRtc: number;
    public uptime: number;
    public nightDetected: number;
    public alertAsserted: number;
    public rawMpptchg: string;
}
