<?php

ini_set('display_errors', '1');
ini_set('display_startup_errors', '1');
error_reporting(E_ALL);

require_once 'common.php';

$data = getLastData('/home/debian/sdcard/data');

?>

<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dashboard - F4HVV-15 / Grand-Ratz (38500)</title>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/@picocss/pico@latest/css/pico.min.css">
</head>
<body>
    <main class="container">
        <hgroup>
            <h1>Dashboard - F4HVV-15 / Grand-Ratz (38500)</h1>
            <p><strong>Dernière relevé :</strong> <span id="derniere_releve"><?= date('d M Y H:i:s', $data['mcu']->time) ?></span></p>
        </hgroup>
        
        <section>
            <hgroup>
                <h2>Caméras</h2>
                <a href="/data/camera" target="_blank">Toutes les photos</a>
            </hgroup>
            <div class="grid">
                <?php foreach ($data['photos'] as $file): ?>
                    <figure>
                        <a href="<?= $file ?>" target="blank">
                            <img width="600" src="<?= $file ?>" />
                        </a>
                    </figure>
                <?php endforeach; ?>
            </div>
        </section>
        
        <div class="grid">
            <section>
                <h2>Énergie</h2>
                <ul>
                    <li><strong>Tension Batterie :</strong> <span id="tension_batterie"><?= $data['mcu']->energy->voltageBattery / 1000 ?> V</span></li>
                    <li><strong>Courant Batterie :</strong> <span id="courant_batterie"><?= $data['mcu']->energy->currentBattery ?> mA</span></li>
                    <li><strong>Tension Solaire :</strong> <span id="tension_solaire"><?= $data['mcu']->energy->voltageSolar / 1000 ?> V</span></li>
                    <!--<li><strong>Courant Solaire :</strong> <span id="courant_solaire"><?= 0 // $data['mcu']->energy->currentSolar ?> mA</span></li>-->
                    <li><strong>Alerte Batterie :</strong> <span id="alerte_batterie"><?= $data['mcu']->box->alertBattery ? 'ALERTE' : 'Ras' ?></span></li>
                    <!-- alertShutdown -->
                </ul>
            </section>
            
            <section>
                <h2>Météo</h2>
                <ul>
                    <li><strong>Température Boîte :</strong> <span id="temp_boite"><?= ($data['mcu']->box->temperatureRtc + $data['mcu']->box->temperatureBattery) / 2 ?> °C</span></li>
                    <!-- temperature -->
                    <?php if (!$data['mcu']->errors->weather): ?>
                        <li><strong>Température Extérieure :</strong> <span id="temp_exterieure"><?= $data['mcu']->weather->temperature ?> °C</span></li>
                        <li><strong>Humidité :</strong> <span id="humidite"><?= $data['mcu']->weather->humidity ?> %</span></li>
                        <li><strong>Pression :</strong> <span id="pression"><?= $data['mcu']->weather->pressure ?> hPa</span></li>
                    <?php endif; ?>
                </ul>
            </section>
            
            <section>
                <h2>Système</h2>          
                <p><strong>Temps de fonctionnement MCU :</strong> <span id="uptime_mcu"><?= secondsToTime($data['mcu']->uptime) ?></span></p>
                <p><strong>Temps de fonctionnement Linux :</strong> <span id="uptime_linux"><?= secondsToTime($data['system']->uptime) ?></span></p>
                <p><strong>CPU Linux :</strong> <span id="cpu_linux"><?= $data['system']->cpu ?> %</span></p>
                <p><strong>RAM Linux :</strong> <span id="ram_linux"><?= $data['system']->ram ?> %</span></p>
                <p><strong>Carte SD Linux :</strong> <span id="sdcard_linux"><?= $data['system']->sdcard ?> %</span></p>
                <!--<p><strong>EMMC Linux :</strong> <span id="disk_linux"><?= $data['system']->disk ?> %</span></p>-->
                
                <?php if ($data['mcu']->errors->energy || $data['mcu']->errors->lora || $data['mcu']->errors->weather): ?>
            <!-- hasErrors -->
                <section>
                    <h3>Erreurs</h3>
                    <ul>
                        <?php if ($data['mcu']->errors->energy): ?>
                            <li id="erreur_energy"><strong>Module energie</strong></li>
                        <?php endif; ?>
                        <?php if ($data['mcu']->errors->lora): ?>
                            <li id="erreur_lora"><strong>Module LoRa</strong></li>
                        <?php endif; ?>
                        <?php if ($data['mcu']->errors->weather): ?>
                            <li id="erreur_meteo"><strong>Capteur météo</strong></li>
                        <?php endif; ?>
                    </ul>
                </section>
            <?php endif; ?>
            </section>
        </div>
        
        <section>
            <h2>Debug</h2>
            
            <div class="grid">
                <p><strong>I2C Meshtastic :</strong> <span id="i2c_meshtastic"><?= secondsToTime($data['mcu']->watchdog->meshtastic->lastFed) ?></span></p>
                <p><strong>I2C MPPT :</strong> <span id="i2c_meshtastic"><?= secondsToTime($data['mcu']->watchdog->mppt->lastFed) ?></span></p>
                <p><strong>LoRa TX :</strong> <span id="lora_tx"><?= secondsToTime($data['mcu']->watchdog->loraTx->lastFed) ?></span></p>
            </div>
            
            <div class="grid">
                <p><strong>TX APRS Position :</strong> <span id="tx_aprs_position"><?= secondsToTime($data['mcu']->aprsSender->sendPositionNextRun) ?></span></p>
                <p><strong>TX APRS Télémétries :</strong> <span id="tx_aprs_telemetries"><?= secondsToTime($data['mcu']->aprsSender->sendTelemetriesNextRun) ?></span></p>
                <p><strong>TX APRS Status :</strong> <span id="tx_aprs_status"><?= secondsToTime($data['mcu']->aprsSender->sendStatusNextRun) ?></span></p>
                <p><strong>TX APRS Meshtastic :</strong> <span id="tx_aprs_meshtastic"><?= secondsToTime($data['mcu']->aprsSender->sendMeshtasticNextRun) ?></span></p>
            </div>
        </section>   
        
        <footer>
            <p><a href="https://valentin-saugnier.fr/qui-suis-je" target="_blank">Valentin SAUGNIER - F4HVV</a> 
            | <a href="https://github.com/valentintintin/hamradio-pi-autonom" target="_blank">GitHub</a> 
            | <a href="https://aprs.fi/info/a/F4HVV-15" target="_blank">Monitoring APRS.fi</a>
            | <a href="https://meshtastic-mqtt-explorer.pixel-server.ovh/node/34091093" target="_blank">Monitoring Meshtastic</a>
            | <a href="https://photos.pixel-server.ovh/share/oZlvtA8_a04PThC3ETYuqNuX20XUe1TpcrWMdR5G1ZTw7HGJFwc4kDCCVoFMwdAvOy8" target="_blank">Photos de l'installation</a></p>
        </footer>
    </main>
</body>
</html>
