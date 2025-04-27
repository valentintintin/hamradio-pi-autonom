<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Affichage des Données SQLite</title>
    <style>
        table {
            width: 100%;
            border-collapse: collapse;
        }
        th, td {
            border: 1px solid #ddd;
            padding: 8px;
        }
        th {
            background-color: #f4f4f4;
        }
        tr:nth-child(even) {
            background-color: #f9f9f9;
        }
        tr:hover {
            background-color: #f1f1f1;
        }
    </style>
</head>
<body>
    <h1>Données de la base SQLite</h1>
    <table>
        <thead>
            <tr>
                <th>Colonne</th>
                <th>Valeur</th>
            </tr>
        </thead>
        <tbody>
            <?php
            // Nom de la base de données SQLite
            $dbFile = '/data/data.db';

            // Vérifier si le fichier de la base existe
            if (!file_exists($dbFile)) {
                echo "<tr><td colspan='2'>Erreur : La base de données SQLite n'existe pas.</td></tr>";
                exit;
            }

            try {
                // Connexion à la base SQLite
                $pdo = new PDO("sqlite:$dbFile");
                $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

                // Requête pour lire les données
                $stmt = $pdo->query("SELECT * FROM telemetry");

                // Vérifier si des données sont disponibles
                if ($stmt->rowCount() == 0) {
                    echo "<tr><td colspan='2'>Aucune donnée disponible.</td></tr>";
                } else {
                    // Parcourir chaque ligne et afficher les données
                    while ($row = $stmt->fetch(PDO::FETCH_ASSOC)) {
                        foreach ($row as $column => $value) {
                            echo "<tr>
                                    <td>$column</td>
                                    <td>$value</td>
                                  </tr>";
                        }
                    }
                }
            } catch (PDOException $e) {
                echo "<tr><td colspan='2'>Erreur : " . htmlspecialchars($e->getMessage()) . "</td></tr>";
            }
            ?>
        </tbody>
    </table>
</body>
</html>

