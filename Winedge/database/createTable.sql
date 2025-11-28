CREATE TABLE WinedgeMVC.dbo.Devices (
	Id int IDENTITY(1,1) NOT NULL,
	DeviceName nvarchar(100) COLLATE Latin1_General_CI_AS NOT NULL,
	Latitude decimal(10,7) NOT NULL,
	Longitude decimal(10,7) NOT NULL,
	TemperatureLimit int NULL,
	HumidityLimit int NULL,
	LuminosityLimit int NULL,
	CONSTRAINT PK_Devices PRIMARY KEY (Id)
);