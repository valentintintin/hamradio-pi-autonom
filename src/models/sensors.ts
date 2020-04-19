import { Entity } from './entity';

export class Sensors extends Entity {
    public createdAt: number = new Date().getTime();
    public voltageBattery: number = -1;
    public voltageSolar: number = -1;
    public currentBattery: number = -1;
    public currentSolar: number = -1;
    public currentCharge: number = -1;
    public temperatureBattery: number = -1;
    public temperatureCpu: number = -1;
    public temperatureRtc: number = -1;
    public uptime: number = -1;
    public nightDetected: number = -1;
    public alertAsserted: number = -1;
    public rawMpptchg: string;
}
