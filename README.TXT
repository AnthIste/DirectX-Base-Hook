Basic outline of how the hook is to work (at this stage):

* Inject dll into remote process
* Create a dummy IDirect3DX object
* Use that object to create a dummy IDirect3DDeviceX object
* Use the dummy object to get location of vtable
* Hook using the vtable
* Delete objects
* Unhook on exit

Notes:

* Creating dummy device might fail
* Git merges seem to be dodgy
