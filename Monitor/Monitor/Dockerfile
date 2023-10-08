FROM mcr.microsoft.com/dotnet/aspnet:7.0-bullseye-slim-arm32v7
RUN apt -y update && apt -y install fswebcam
WORKDIR /App
#COPY . .
ENTRYPOINT ["dotnet", "Monitor.dll"]