<!DOCTYPE html>
<html>
<head>
    <title>Graphiques de données</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
    <canvas id="mpptChart" width="400" height="400"></canvas>

    <?php
    $db = new SQLite3('data.db'); // Remplacez 'your_database.db' par le chemin de votre base de données

    // Récupération des données de la table Mppts
    $mpptResults = $db->query('SELECT * FROM Mppts WHERE Mod(Id, 60) = 0');
    $mpptLabels = [];
    $batteryVoltages = [];
    $batteryCurrents = [];
    $solarVoltages = [];
    $solarCurrents = [];

    while ($row = $mpptResults->fetchArray()) {
        $mpptLabels[] = $row['CreatedAt'];
        $batteryVoltages[] = $row['BatteryVoltage'] / 1000;
        $batteryCurrents[] = $row['BatteryCurrent'];
        $solarVoltages[] = $row['SolarVoltage'] / 1000;
        $solarCurrents[] = $row['SolarCurrent'];
    }
    ?>

    <script>
        // Graphique pour les données MPPT
        var mpptCtx = document.getElementById('mpptChart').getContext('2d');
        var mpptChart = new Chart(mpptCtx, {
            type: 'line',
            data: {
                labels: <?php echo json_encode($mpptLabels); ?>,
                datasets: [{
                    label: 'Battery Voltage',
                    data: <?php echo json_encode($batteryVoltages); ?>,
                    borderColor: 'rgba(255, 206, 86, 1)',
                    borderWidth: 1,
                    fill: false,
                    yAxisID: 'y',
                }, {
                    label: 'Battery Current',
                    data: <?php echo json_encode($batteryCurrents); ?>,
                    borderColor: 'rgba(75, 192, 192, 1)',
                    borderWidth: 1,
                    fill: false,
                    yAxisID: 'y1',
                }, {
                    label: 'Solar Voltage',
                    data: <?php echo json_encode($solarVoltages); ?>,
                    borderColor: 'rgba(153, 102, 255, 1)',
                    borderWidth: 1,
                    fill: false,
                    yAxisID: 'y',
                }, {
                    label: 'Solar Current',
                    data: <?php echo json_encode($solarCurrents); ?>,
                    borderColor: 'rgba(255, 159, 64, 1)',
                    borderWidth: 1,
                    fill: false,
                    yAxisID: 'y1',
                }]
            },
            options: {
                scales: {
                    y: {
                        beginAtZero: true,
                        title: {
                          display: true,
                          text: 'Tension (V)'
                        },
                    },
                      y1: {
                        title: {
                          display: true,
                          text: 'Courant (mA)'
                        },
                        type: 'linear',
                        display: true,
                        position: 'right',
                        beginAtZero: true
                      },
                }
            }
        });
    </script>
</body>
</html>

