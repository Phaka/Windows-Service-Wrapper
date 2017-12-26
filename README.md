# Phaka Windows Service Wrapper

A windows service application that wraps a process, e.g. a Java application or .NET core.

## Usage

Let's assume  you have a ASP.NET Core application compiled as myapp.dll that you would like to run as a Windows Service. 

1. Download the latest build from the releases folder
2. Unzip it to a local path, e.g. c:\myapp
3. Rename wrapper.exe to myapp.exe  
4. Create a configuration file myapp.cfg that looks as follows:

        [Application]
        CommandLine=dotnet myapp.dll

        [Service]
        Name=myapp
        DisplayName=My Application
        Description=A service that tries to find the question to 42

5. Run the application as a normal application

        myapp 

6. Install the Windows Service

        myapp install

7. Unininstall the Windows Service 

        myapp uninstall

8. Disable the Windows Service

        myapp disable

9. Query the Windows Service

        myapp query

10. Start the Windows Service

        myapp start

11. Stop the Windows Service

        myapp stop


