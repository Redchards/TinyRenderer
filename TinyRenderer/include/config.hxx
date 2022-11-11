#ifndef TINYRENDERER_CONFIG_HXX
#define TINYRENDERER_CONFIG_HXX

#ifdef DLL_EXPORT
#define DLL_API __declspec(dllexport) 
#else
#define DLL_API __declspec(dllimport)  
#endif

#endif // TINYRENDERER_CONFIG_HXX
