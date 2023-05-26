using System;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace Monitor.Migrations
{
    /// <inheritdoc />
    public partial class Initial : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.CreateTable(
                name: "Mppts",
                columns: table => new
                {
                    Id = table.Column<long>(type: "INTEGER", nullable: false)
                        .Annotation("Sqlite:Autoincrement", true),
                    CreatedAt = table.Column<DateTime>(type: "TEXT", nullable: false),
                    BatteryVoltage = table.Column<int>(type: "INTEGER", nullable: false),
                    BatteryCurrent = table.Column<int>(type: "INTEGER", nullable: false),
                    SolarVoltage = table.Column<int>(type: "INTEGER", nullable: false),
                    SolarCurrent = table.Column<int>(type: "INTEGER", nullable: false),
                    CurrentCharge = table.Column<int>(type: "INTEGER", nullable: false),
                    Status = table.Column<int>(type: "INTEGER", nullable: false),
                    Night = table.Column<bool>(type: "INTEGER", nullable: false),
                    Alert = table.Column<bool>(type: "INTEGER", nullable: false),
                    WatchdogEnabled = table.Column<bool>(type: "INTEGER", nullable: false),
                    WatchdogPowerOffTime = table.Column<long>(type: "INTEGER", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_Mppts", x => x.Id);
                });

            migrationBuilder.CreateTable(
                name: "Weathers",
                columns: table => new
                {
                    Id = table.Column<long>(type: "INTEGER", nullable: false)
                        .Annotation("Sqlite:Autoincrement", true),
                    CreatedAt = table.Column<DateTime>(type: "TEXT", nullable: false),
                    Temperature = table.Column<double>(type: "REAL", nullable: false),
                    Humidity = table.Column<int>(type: "INTEGER", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_Weathers", x => x.Id);
                });
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "Mppts");

            migrationBuilder.DropTable(
                name: "Weathers");
        }
    }
}
