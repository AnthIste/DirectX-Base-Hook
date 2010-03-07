1. Basic outline of how the hook is to work (at this stage):
 
* Inject dll into remote process
* See if d3d9.dll is loaded already
* If it is, do a runtime hook on DirectX (2.a)
* If it isnt, see if the target has Direct3DCreate9 in the IAT
* If it does, hook Direct3DCreate9 (2.b)
* If it doesnt hook GetProcAddress (2.c)

2.a Runtime hook

* Create a dummy IDirect3DDevice9 object (perhaps in a dummy window)
* Use this object to get the device vtable
* Hook using the vtable
* Destroy dummy object / window

2.b Standard hook

* When Direct3DCreate9 is called, hook CreateDevice using the d3d object's vtable
* When CreateDevice is called, hook using the device's vtable

2.c Dynamic hook

* If the target loads d3d9.dll dynamically, it needs to call GetProcAddress on Direct3DCreate9
* When it asks for the address of Direct3DCreate9, pass it the address of our detour instead
* Using the GetProcAddress detour, proceed as if its a standard hook (2.b)