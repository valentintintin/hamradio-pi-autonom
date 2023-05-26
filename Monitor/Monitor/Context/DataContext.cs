using Microsoft.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore.ChangeTracking;
using Monitor.Context.Entities;

namespace Monitor.Context;

public class DataContext : DbContext
{
    public required DbSet<Weather> Weathers { get; set; }
    
    public required DbSet<Mppt> Mppts { get; set; }
    
    public required DbSet<Entities.System> Systems { get; set; }
    
    public DataContext(DbContextOptions<DataContext> options) : base(options) {}

    public override int SaveChanges()
    {
        ComputeEntitiesBeforeSaveChanges();
        
        return base.SaveChanges();
    }

    private void ComputeEntitiesBeforeSaveChanges()
    {
        foreach (EntityEntry entityEntry in ChangeTracker.Entries())
        {
            if (entityEntry.Entity is IEntity entity)
            {
                switch (entityEntry.State)
                {
                    case EntityState.Added:
                        entity.CreatedAt = DateTime.UtcNow;
                        break;

                }
            }
        }
    }
}