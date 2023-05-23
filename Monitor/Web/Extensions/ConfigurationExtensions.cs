using System.Diagnostics.CodeAnalysis;
using Web.Exceptions;

namespace Web.Extensions;

public static class ConfigurationExtensions
{
    public static T GetValueOrThrow<[DynamicallyAccessedMembers(DynamicallyAccessedMemberTypes.All)] T>(this IConfiguration configuration, string key)
    {
        T? value = (T?)configuration.GetValue(typeof(T), key);

        if (value == null)
        {
            throw new MissingConfigurationException(key);
        }

        return value;
    }
}