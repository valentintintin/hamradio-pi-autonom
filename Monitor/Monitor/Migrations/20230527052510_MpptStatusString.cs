using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace Monitor.Migrations
{
    /// <inheritdoc />
    public partial class MpptStatusString : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.AddColumn<string>(
                name: "StatusString",
                table: "Mppts",
                type: "TEXT",
                nullable: true);
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "StatusString",
                table: "Mppts");
        }
    }
}
