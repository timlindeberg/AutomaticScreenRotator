# AutomaticScreenRotator
An application to automatically set the display mode when the screen is rotated.

The application works by attaching an old iPhone to the back of screen:

![alt tag](http://i.imgur.com/F2c4EwK.jpg =100x)

The iPhone runs an app (RotationUpdater) which sends it's rotation mode (portrait or landscape) over a socket to the server (RotatorServer).
When the server recieves a rotation update from the phone it sets the corresponding rotation mode for the screen. The server currently only has a few settings specified in the settings file. You can specify the screen index to use and the position in the virtual desktop that the screen should have in portrait and landscape mode. 

The result can be seen here:
https://youtu.be/x90C_2l0xto
