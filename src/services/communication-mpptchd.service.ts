import { Observable, Observer } from 'rxjs';
import { LogService } from './log.service';
import { delay, map, retryWhen, switchMap, take } from 'rxjs/operators';
import i2c = require('i2c-bus');

export enum CommandMpptChd {
    ID = 0,
    STATUS = 2,
    BUCK = 4,
    VS = 6,
    IS = 8,
    VB = 10,
    IB = 12,
    IC = 14,
    IT = 16,
    ET = 18,
    VM = 20,
    TH = 22,
    BULKV = 24,
    FLOATV = 26,
    PWROFFV = 28,
    PWRONV = 30,
    WDEN = 33,
    WDCNT = 35,
    WDPWROFF = 36
}

export enum StatusMpptChd {
    CHARGER_STATE_NIGHT = 0x0000,
    CHARGER_STATE_IDLE = 0x0001,
    CHARGER_STATE_VSCR = 0x0002,
    CHARGER_STATE_SCAN = 0x0003,
    CHARGER_STATE_BULK = 0x0004,
    CHARGER_STATE_ABSORPTION = 0x0005,
    CHARGER_STATE_FLOAT = 0x0006,
    NIGHT_DETECTED = 0x0008,
    CHARGER_TEMERATURE_LIMIT = 0x0010,
    POWER_ENABLE_JUMPER = 0x0020,
    ALERT_ASSERTED = 0x0040,
    POWER_ENABLED = 0x0080,
    WATCHDOG_RUNNING = 0x0100,
    EXTERNAL_TEMPERATURE_SENSOR_MISSING = 0x1000,
    BAD_BATTERY_STATUS = 0x2000,
    POWER_WATCHDOG_TRIGGERED = 0x4000,
    INTERNAL_WATCHDOG_TRIGGERED_ = 0x8000
}

export interface MpptChdStatusInterface {
    raw: number[];
    chargerState: MpptChdStatusChargerStateInterface, // 2:0
    nightDetected: boolean; // 3
    chargerTemperatureLimit: boolean; // 4
    powerEnabledJumper: boolean; //5
    alertAsserted: boolean; // 6
    powerEnabled: boolean; // 7
    watchdogRunning: boolean; // 8
    // 11:9 reserved
    externalTemperatureSensorMissing: boolean; // 12
    badBatteryStatus: boolean; // 13
    powerWatchdogTriggered: boolean; // 14
    internalWatchdogTriggered: boolean; // 15
    values?: MpptChdValuesInterface
}

export interface MpptChdStatusChargerStateInterface {
    night: boolean; // 0
    idle: boolean; // 1
    vsrcv: boolean; // 2
    scan: boolean; // 3
    bulk: boolean; // 4
    absorption: boolean; //5
    float: boolean; //6
}

export interface MpptChdValuesInterface {
    batteryVoltage: number;
    batteryCurrent: number;
    solarVoltage: number;
    solarCurrent: number;
    chargeCurrent: number;
    internalThermometer: number;
    externalThermometer: number;
    currentMpptSolarVolate: number;
    currentBatteryChargeSetVoltage: number;
    bulkVolage: number;
    floatVoltage: number;
    powerOffVoltage: number;
    powerOnVoltage: number;
    watchdogEnable: number;
    watchdogPowerOff: number;
    watchdogCounter: number;
}

export interface MpptChdStatusBuckInterface {
    buckLimit1: boolean; // 0
    buckLimit2: boolean; // 1
    // 5:2 reserved
    chargerBuckConverterPwmValue: number; // 15:6
}

class Command {
    constructor(
        public name: string,
        public isWritable: boolean,
        public isWord: boolean,
        public isSigned: boolean,
        public regAddr: number,
        public fake: number = 1,
    ) {
    }
}

export class CommunicationMpptchdService {

    public static USE_FAKE = false;
    public static DEBUG = false;

    private static readonly COMMANDS: Command[] = [
        new Command('ID', false, true, false, CommandMpptChd.ID, 0x1000),
        new Command('STATUS', false, true, false, CommandMpptChd.STATUS, CommunicationMpptchdService.getStatusData({
            raw: [0, 0],
            chargerState: {
                night: false,
                idle: false,
                vsrcv: false,
                scan: false,
                bulk: false,
                absorption: false,
                float: false
            },
            nightDetected: false,
            chargerTemperatureLimit: false,
            powerEnabledJumper: false,
            alertAsserted: false,
            powerEnabled: true,
            watchdogRunning: false,
            externalTemperatureSensorMissing: false,
            badBatteryStatus: false,
            powerWatchdogTriggered: false,
            internalWatchdogTriggered: false
        })),
        new Command('BUCK', false, true, false, CommandMpptChd.BUCK, 0),
        new Command('VS', false, true, false, CommandMpptChd.VS, 20720),
        new Command('IS', false, true, false, CommandMpptChd.IS, 600),
        new Command('VB', false, true, false, CommandMpptChd.VB, 12500),
        new Command('IB', false, true, false, CommandMpptChd.IB, 450),
        new Command('IC', false, true, true, CommandMpptChd.IC, 470),
        new Command('IT', false, true, true, CommandMpptChd.IT, 100),
        new Command('ET', false, true, true, CommandMpptChd.ET, 10),
        new Command('VM', false, true, false, CommandMpptChd.VM, 14321),
        new Command('TH', false, true, false, CommandMpptChd.TH, 0),
        new Command('BULKV', true, true, false, CommandMpptChd.BULKV, 13120),
        new Command('FLOATV', true, true, false, CommandMpptChd.FLOATV, 11000),
        new Command('PWROFFV', true, true, false, CommandMpptChd.PWROFFV, 11500),
        new Command('PWRONV', true, true, false, CommandMpptChd.PWRONV, 12500),
        new Command('WDEN', true, false, false, CommandMpptChd.WDEN, 0),
        new Command('WDCNT', true, false, false, CommandMpptChd.WDCNT, 0),
        new Command('WDPWROFF', true, true, false, CommandMpptChd.WDPWROFF, 0)
    ];

    private static i2c;
    private static _instance: CommunicationMpptchdService;

    private static readonly WDEN_MAGIC_BYTE = 0xEA;

    public static get instance(): CommunicationMpptchdService {
        if (!this._instance) {
            if (!CommunicationMpptchdService.USE_FAKE) {
                CommunicationMpptchdService.i2c = i2c.openSync(0);
            }
            this._instance = new CommunicationMpptchdService();
        }
        return this._instance;
    }

    private readonly i2cAddr: number;

    constructor() {
        this.i2cAddr = 0x12;

        this.receive(CommandMpptChd.ID).subscribe(value => {
            if ((value & 0xF000) !== 0x1000) {
                LogService.log('i2c-' + 'mpptChg', 'Charger not detected !', value);
                throw new Error('Charger did not identify correctly');
            }
        });
    }

    public getStatus(): Observable<MpptChdStatusInterface> {
        // @ts-ignore
        return this.receive(CommandMpptChd.STATUS).pipe(
            switchMap(status => this.receive(CommandMpptChd.STATUS).pipe(
                map(status2 => {
                    return {
                        raw: [status, status2],
                        chargerState: {
                            night: !!CommunicationMpptchdService.getData(StatusMpptChd.CHARGER_STATE_NIGHT, status, status2),
                            idle: !!CommunicationMpptchdService.getData(StatusMpptChd.CHARGER_STATE_IDLE, status, status2),
                            vsrcv: !!CommunicationMpptchdService.getData(StatusMpptChd.CHARGER_STATE_VSCR, status, status2),
                            scan: !!CommunicationMpptchdService.getData(StatusMpptChd.CHARGER_STATE_SCAN, status, status2),
                            bulk: !!CommunicationMpptchdService.getData(StatusMpptChd.CHARGER_STATE_BULK, status, status2),
                            absorption: !!CommunicationMpptchdService.getData(StatusMpptChd.CHARGER_STATE_ABSORPTION, status, status2),
                            float: !!CommunicationMpptchdService.getData(StatusMpptChd.CHARGER_STATE_FLOAT, status, status2),
                        },
                        nightDetected: !!CommunicationMpptchdService.getData(StatusMpptChd.NIGHT_DETECTED, status, status2),
                        chargerTemperatureLimit: !!CommunicationMpptchdService.getData(StatusMpptChd.CHARGER_TEMERATURE_LIMIT, status, status2),
                        powerEnabledJumper: !!CommunicationMpptchdService.getData(StatusMpptChd.POWER_ENABLE_JUMPER, status, status2),
                        alertAsserted: !!CommunicationMpptchdService.getData(StatusMpptChd.ALERT_ASSERTED, status, status2),
                        powerEnabled: !!CommunicationMpptchdService.getData(StatusMpptChd.POWER_ENABLED, status, status2),
                        watchdogRunning: !!CommunicationMpptchdService.getData(StatusMpptChd.WATCHDOG_RUNNING, status, status2),
                        externalTemperatureSensorMissing: !!CommunicationMpptchdService.getData(StatusMpptChd.EXTERNAL_TEMPERATURE_SENSOR_MISSING, status, status2),
                        badBatteryStatus: !!CommunicationMpptchdService.getData(StatusMpptChd.BAD_BATTERY_STATUS, status, status2),
                        powerWatchdogTriggered: !!CommunicationMpptchdService.getData(StatusMpptChd.POWER_WATCHDOG_TRIGGERED, status, status2),
                        internalWatchdogTriggered: !!CommunicationMpptchdService.getData(StatusMpptChd.INTERNAL_WATCHDOG_TRIGGERED_, status, status2),
                        values: {}
                    } as MpptChdStatusInterface;
                })
            )),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.VB).pipe(map(value => {
                    status.values.batteryVoltage = value;
                    return status;
                }))
            }),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.IB).pipe(map(value => {
                    status.values.batteryCurrent = value;
                    return status;
                }))
            }),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.VS).pipe(map(value => {
                    status.values.solarVoltage = value;
                    return status;
                }))
            }),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.IS).pipe(map(value => {
                    status.values.solarCurrent = value;
                    return status;
                }))
            }),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.IC).pipe(map(value => {
                    status.values.chargeCurrent = value;
                    return status;
                }))
            }),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.IT).pipe(map(value => {
                    status.values.internalThermometer = value;
                    return status;
                }))
            }),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.ET).pipe(map(value => {
                    status.values.externalThermometer = value;
                    return status;
                }))
            }),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.VM).pipe(map(value => {
                    status.values.currentMpptSolarVolate = value;
                    return status;
                }))
            }),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.TH).pipe(map(value => {
                    status.values.currentBatteryChargeSetVoltage = value;
                    return status;
                }))
            }),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.BULKV).pipe(map(value => {
                    status.values.bulkVolage = value;
                    return status;
                }))
            }),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.FLOATV).pipe(map(value => {
                    status.values.floatVoltage = value;
                    return status;
                }))
            }),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.PWROFFV).pipe(map(value => {
                    status.values.powerOffVoltage = value;
                    return status;
                }))
            }),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.PWRONV).pipe(map(value => {
                    status.values.powerOnVoltage = value;
                    return status;
                }))
            }),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.WDEN).pipe(map(value => {
                    status.values.watchdogEnable = value;
                    return status;
                }))
            }),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.WDCNT).pipe(map(value => {
                    status.values.watchdogCounter = value;
                    return status;
                }))
            }),
            switchMap((status: MpptChdStatusInterface) => {
                return CommunicationMpptchdService.instance.receive(CommandMpptChd.WDPWROFF).pipe(map(value => {
                    status.values.watchdogPowerOff = value;
                    return status;
                }))
            }),
        );
    }

    public enableWatchdog(powerOffSecs: number, initSecs: number): Observable<void> {
        return this.send(CommandMpptChd.WDPWROFF, powerOffSecs).pipe(
            switchMap(_ => this.send(CommandMpptChd.WDCNT, initSecs)),
            switchMap(_ => this.send(CommandMpptChd.WDEN, CommunicationMpptchdService.WDEN_MAGIC_BYTE)),
        );
    }

    public disableWatchdog(): Observable<void> {
        return this.send(CommandMpptChd.WDEN, 0);
    }

    public send(command: CommandMpptChd, data: number = 0): Observable<void> {
        return new Observable<void>((observer: Observer<void>) => {
            const commandObject: Command = CommunicationMpptchdService.COMMANDS.find(c => c.regAddr === command);
            if (!commandObject) {
                LogService.log('i2c-' + 'mpptChg', 'Send command KO. Command does not exist', commandObject.name, data);
                observer.error(new Error('Command does not exist'));
            }
            if (!commandObject.isWritable) {
                LogService.log('i2c-' + 'mpptChg', 'Send command KO. Command is not writable', commandObject.name, data);
                observer.error(new Error('Command is not writable'));
            }

            if (CommunicationMpptchdService.DEBUG) {
                LogService.log('i2c-' + 'mpptChg', 'Send command', { command: commandObject.name, data: data });
            }

            let receive = 0;
            if (!CommunicationMpptchdService.USE_FAKE) {
                try {
                    if (commandObject.isWord) {
                        receive = CommunicationMpptchdService.i2c.writeWordSync(this.i2cAddr, commandObject.regAddr, ((data >> 8) & 0xFF) | ((data & 0xFF) << 8));
                    } else {
                        if (data > 255) {
                            observer.error(new Error(`Data not 8 bits : ${data} !`))
                        }
                        receive = CommunicationMpptchdService.i2c.writeByteSync(this.i2cAddr, commandObject.regAddr, data & 0xFF);
                    }
                } catch (e) {
                    LogService.log('i2c-' + 'mpptChg', 'Send command KO', commandObject.name, data, e);
                    observer.error(e);
                }
            }

            if (receive === -1) {
                LogService.log('i2c-' + 'mpptChg', 'Send command KO', commandObject.name, data);
                observer.error(new Error('Error during i2c write'));
            }

            observer.next();
            observer.complete();
        }).pipe(
            retryWhen(errors => errors.pipe(delay(250), take(3)))
        );
    }

    public receive(command: CommandMpptChd): Observable<number> {
        return new Observable<number>((observer: Observer<number>) => {
            const commandObject: Command = CommunicationMpptchdService.COMMANDS.find(c => c.regAddr === command);
            if (!commandObject) {
                LogService.log('i2c-' + 'mpptChg', 'Receive KO. Command does not exist', command);
                observer.error(new Error('Command does not exist'));
            }

            if (CommunicationMpptchdService.DEBUG) {
                LogService.log('i2c-' + 'mpptChg', 'Receive', commandObject.name);
            }

            let received = commandObject.fake;
            if (!CommunicationMpptchdService.USE_FAKE) {
                try {
                    if (commandObject.isWord) {
                        received = CommunicationMpptchdService.i2c.readWordSync(this.i2cAddr, commandObject.regAddr);
                        if (received !== -1) {
                            received = ((received & 0xFF) << 8) | ((received >> 8) & 0xFF);
                        }
                    } else {
                        received = CommunicationMpptchdService.i2c.readByteSync(this.i2cAddr, commandObject.regAddr);
                    }
                } catch (e) {
                    LogService.log('i2c-' + 'mpptChg', 'Send command KO', commandObject.name, e);
                    observer.error(e);
                }
            }

            if (received === -1) {
                LogService.log('i2c-' + 'mpptChg', 'Receive KO', commandObject.name);
                observer.error(new Error('Receive KO'));
            } else if (commandObject.isSigned) {
                // 2's complement to create negative int
                if (commandObject.isWord) {
                    if (received & 0x8000) {
                        received = -(0x8000 - (received & 0x7FFF));
                    }
                } else {
                    if (received & 0x80) {
                        received = -(0x80 - (received & 0x7F));
                    }
                }
            }

            if (CommunicationMpptchdService.DEBUG) {
                LogService.log('i2c-' + 'mpptChg', 'Receive response', {
                    command: commandObject.name,
                    received: received
                });
            }

            observer.next(received);
            observer.complete();
        }).pipe(
            retryWhen(errors => errors.pipe(delay(250), take(3)))
        );
    }

    private static getData(mask: number, value1: number, value2: number = null, error: number = 0): number {
        if (value2 !== null) {
            return (value1 & mask) === (value2 & mask) ? value1 & mask : error;
        }
        return value1 & mask;
    }

    private static getStatusData(config: MpptChdStatusInterface): number {
        let value = config.chargerState.night ? StatusMpptChd.CHARGER_STATE_NIGHT : 0;
        value += config.chargerState.idle ? StatusMpptChd.CHARGER_STATE_IDLE : 0;
        value += config.chargerState.vsrcv ? StatusMpptChd.CHARGER_STATE_VSCR : 0;
        value += config.chargerState.scan ? StatusMpptChd.CHARGER_STATE_SCAN : 0;
        value += config.chargerState.bulk ? StatusMpptChd.CHARGER_STATE_BULK : 0;
        value += config.chargerState.absorption ? StatusMpptChd.CHARGER_STATE_ABSORPTION : 0;
        value += config.chargerState.float ? StatusMpptChd.CHARGER_STATE_FLOAT : 0;
        value += config.nightDetected ? StatusMpptChd.NIGHT_DETECTED : 0;
        value += config.chargerTemperatureLimit ? StatusMpptChd.CHARGER_TEMERATURE_LIMIT : 0;
        value += config.powerEnabledJumper ? StatusMpptChd.POWER_ENABLE_JUMPER : 0;
        value += config.alertAsserted ? StatusMpptChd.ALERT_ASSERTED : 0;
        value += config.powerEnabled ? StatusMpptChd.POWER_ENABLED : 0;
        value += config.watchdogRunning ? StatusMpptChd.WATCHDOG_RUNNING : 0;
        value += config.externalTemperatureSensorMissing ? StatusMpptChd.EXTERNAL_TEMPERATURE_SENSOR_MISSING : 0;
        value += config.badBatteryStatus ? StatusMpptChd.BAD_BATTERY_STATUS : 0;
        value += config.powerWatchdogTriggered ? StatusMpptChd.POWER_WATCHDOG_TRIGGERED : 0;
        value += config.internalWatchdogTriggered ? StatusMpptChd.INTERNAL_WATCHDOG_TRIGGERED_ : 0;
        return value;
    }
}
