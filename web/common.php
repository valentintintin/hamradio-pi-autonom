<?php

date_default_timezone_set('Europe/Paris');

function getLastData($directory) {
    return [
        'photos' => array_map(function(string $path) { 
            return '/data/camera' . $path; 
        }, array_values(getLastFileInEachSubfolder($directory . '/camera'))),
        'system' => json_decode(file_get_contents($directory . '/system.json')),
        'mcu' => json_decode(file_get_contents($directory . '/mcu.json')),
    ];
}

function getLastFileInEachSubfolder($directory) {
    $result = [];

    // Liste tous les sous-dossiers
    foreach (glob($directory . '/*', GLOB_ONLYDIR) as $subfolder) {
        $files = glob($subfolder . '/*'); // Récupère tous les fichiers du sous-dossier

        if (!empty($files)) {
            // Trie les fichiers par date de modification (du plus récent au plus ancien)
            usort($files, function ($a, $b) {
                return filemtime($b) - filemtime($a);
            });

            // Ajoute le dernier fichier trouvé au résultat
            $result[$subfolder] = str_replace($directory, '', $files[0]);
        }
    }

    return $result;
}

function secondsToTime($seconds) {
    $days = floor($seconds / 86400);
    $hours = floor(($seconds % 86400) / 3600);
    $minutes = floor(($seconds % 3600) / 60);
    $seconds = $seconds % 60;

    if ($days == 0) {
        return sprintf("%02d:%02d:%02d", $hours, $minutes, $seconds);
    }
    
    return sprintf("%02d jours %02d:%02d:%02d", $days, $hours, $minutes, $seconds);
}
