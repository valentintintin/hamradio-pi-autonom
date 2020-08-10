import { Entity } from './entity';

export class Sensors extends Entity {
    public createdAt: number = new Date().getTime();
    public voltageBattery: number = null;
    public voltageSolar: number = null;
    public currentBattery: number = null;
    public currentSolar: number = null;
    public currentCharge: number = null;
    public temperatureBattery: number = null;
    public temperatureCpu: number = null;
    public temperatureRtc: number = null;
    public uptime: number = null;
    public nightDetected: number = null;
    public alertAsserted: number = null;
    public rawMpptchg: string;
}
