1. Basic outline of how the hook is to work (at this stage):
 
* Inject dll into remote process
* See if d3d9.dll is loaded already
* If it is, do a runtime hook on DirectX (2.a)
* If it isnt:
* Hook Direct3DCreate9 (2.b) and
* Hook GetProcAddress (2.c)

2.a Runtime hook

* Create a dummy IDirect3DDevice9 object (perhaps in a dummy window)
* Use this object to get the device vtable
* Hook using the vtable
* Destroy dummy object / window

2.b Standard hook

* When Direct3DCreate9 is called, hook CreateDevice using the d3d object's vtable
* When CreateDevice is called, hook using the device's vtable
* Remove the GetProcAddress hook, just to be safe

2.c Dynamic hook

* If the target loads d3d9.dll dynamically, it needs to call GetProcAddress on Direct3DCreate9
* When it asks for the address of Direct3DCreate9:
* Remove the Direct3DCreate9 detour, just to be safe
* Pass it the address of our Direct3DCreate9 detour instead
* Using the Direct3DCreate9 detour, proceed as if its a standard hook (2.b)