# RayTracer
Simple ray tracer build in pure C.

## Samples

### 256 Rays per Pixel, 1000 Ray Counts
![Test Image 1](https://user-images.githubusercontent.com/56222543/217295338-0efecda9-f626-4c71-a151-8e24dd177460.jpg)

### 500 Rays per Pixel with Multi-Threading Support
![Test Image 2](https://user-images.githubusercontent.com/56222543/233251704-b224f793-3128-4b61-a501-f288edb8c9fa.jpg)

## Build guide for windows
1. Open your command prompt and type:
    ```batch
    .\generate_project.bat --help
    ```

2. Generate the Visual Studio 2022 files:
    ```batch
    .\generate_project.bat vs2022
    ```

3. Navigate to the RayTracer folder through your file explorer, and open the .sln file.

4. Using Visual studio build and run the project.
