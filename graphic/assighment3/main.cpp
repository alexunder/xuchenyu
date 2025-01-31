/*
 *  The real first assignment of graphic
 *  Developed by Mohist Research
 */
#include<stdio.h>
#include<string.h>
#include<GL/glut.h>

#include "scene_parser.h"
#include "OrthographicCamera.h" 
#include "PerspectiveCamera.h"
#include "Group.h"
#include "image.h"
#include "hit.h"
#include "material.h"
#include "light.h" 
#include "glCanvas.h"
#include "Sphere.h"

char * input_file = NULL;
int    width = 100;
int    height = 100;
char * output_file = NULL;
float  depth_min = 0;
float  depth_max = 1;
char * depth_file = NULL;
char * normal_file = NULL;
bool needShadeBack = false;
int theta_steps = 10;
int phi_steps = 5;
bool guiMode = false;
SceneParser * parser = NULL;
//Camera * pCamera = NULL;

void parseArgs(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) 
    {
        if (!strcmp(argv[i],"-input")) 
        {
            i++; 
            assert (i < argc); 
            input_file = argv[i];
        } 
        else if (!strcmp(argv[i],"-size")) 
        {
            i++; 
            assert (i < argc); 
            width = atoi(argv[i]);
            i++; 
            assert (i < argc); 
            height = atoi(argv[i]);
        } 
        else if (!strcmp(argv[i],"-output")) 
        {
            i++;
            assert (i < argc); 
            output_file = argv[i];
        } 
        else if (!strcmp(argv[i],"-depth")) 
        {
            i++;
            assert (i < argc); 
            depth_min = atof(argv[i]);
            i++;
            assert (i < argc); 
            depth_max = atof(argv[i]);
            i++;
            assert (i < argc); 
            depth_file = argv[i];
        }
        else if (!strcmp(argv[i], "-normals"))
        {
            i++;
            assert (i < argc);
            normal_file = argv[i];
        }
        else if (!strcmp(argv[i], "-shade_back"))
        {
            needShadeBack = true;
        }
        else if (!strcmp(argv[i],"-tessellation"))
        {
            i++;
            assert (i < argc);
            theta_steps = atof(argv[i]);
            i++;
            assert (i < argc);
            phi_steps = atof(argv[i]);
        }
        else if (!strcmp(argv[i], "-gui"))
        {
            guiMode = true;
        }
        else 
        {
            printf ("whoops error with command line argument %d: '%s'\n",i,argv[i]);
            assert(0);
        }
    }
}

void RenderScene()
{


	Camera * pCamera = parser->getCamera();

    if (pCamera == NULL)
    {
        fprintf(stderr, "The camera is not defined in config file, so quit.");
        return;
    }
    
    if (pCamera->getCameraType() == CameraType::Orthographic)
    {
#ifdef DEBUG
        printf("The Camera type is Orthographic.\n");
#endif
        // crop the output image, to make sure the rectangle is the squre.
        if (width < height)
        {
            height = width;
        }
        else if (width > height)
        {
            width = height;
        }
    }
    else if (pCamera->getCameraType() == CameraType::Perspective)
    {
#ifdef DEBUG
        printf("The Camera type is Perspective.\n");
#endif
        float ratio = ((float)width) / ((float)height);
        ((PerspectiveCamera*)pCamera)->setRatio(ratio);
    }

    Vec3f backColor = parser->getBackgroundColor();
	Vec3f ambientLight = parser->getAmbientLight();

    Group * objGroups = parser->getGroup();

    int numberLights = parser->getNumLights();

    Image outImg(width, height);
	outImg.SetAllPixels(backColor);

	//Generate the image content
	int i;
	int j;
    float tmin = pCamera->getTMin();

#ifdef DEBUG
	printf("tmin=%f\n", tmin);
#endif

	for (j = 0; j < height; j++)
	for (i = 0; i < width; i++)
	{
        float u = (i + 0.5) / width;
        float v = (j + 0.5) / height;
        Vec2f p(u, v);
	    Ray	r = pCamera->generateRay(p);

        bool ishit = false;
        Hit h;
        ishit = objGroups->intersect(r, h, tmin);

        if (ishit)
        {
#ifdef DEBUG
        printf("Pixel(%d, %d), t=%f\n", i, j, h.getT());
#endif
            Material * pM = h.getMaterial();
            Vec3f normal = h.getNormal();
            Vec3f point = h.getIntersectionPoint();

            Vec3f diffuseColor = pM->getDiffuseColor();

            Vec3f pixelColor(diffuseColor[0] * ambientLight[0],
                             diffuseColor[1] * ambientLight[1],
                             diffuseColor[2] * ambientLight[2] );



            Vec3f delata(0.0, 0.0, 0.0);

            int k;
            for (k = 0; k < numberLights; k++)
            {
                Light * plight = parser->getLight(k);

                if (plight == NULL)
                    continue;

                Vec3f lightDir;
                Vec3f lightColor;
                float distance;
                plight->getIllumination(point, lightDir, lightColor, distance);
                float d = lightDir.Dot3(normal);

                if (d < 0)
                    d = 0.0;

                Vec3f temp(lightColor[0] * diffuseColor[0],
                           lightColor[1] * diffuseColor[1],
                           lightColor[2] * diffuseColor[2]);

                temp = d * temp;

                delata += temp;
                delata += pM->Shade(r, h, lightDir, lightColor);
            }

            pixelColor += delata;
            pixelColor.Clamp();
            outImg.SetPixel(i, j, pixelColor);
        }
	}

	outImg.SaveTGA(output_file);
}

void RenderDepth()
{

    if (depth_file == NULL)
    {
        fprintf(stderr, "Depth file is null, no need to render.\n");
    }
    else
    {
        Vec3f backColor = parser->getBackgroundColor();
        Image depthtImg(width, height);
        depthtImg.SetAllPixels(backColor);

        Camera * pCamera = parser->getCamera();
        Group * objGroups = parser->getGroup();

        if (pCamera == NULL)
        {
            fprintf(stderr, "The camera is not defined in config file, so quit.");
            return;
        }

        int i;
        int j;
        float tmin = pCamera->getTMin();

        for (j = 0; j < height; j++)
            for (i = 0; i < width; i++)
            {
                float u = (i + 0.5) / width;
                float v = (j + 0.5) / height;
                Vec2f p(u, v);
                Ray	r = pCamera->generateRay(p);

                bool ishit = false;
                Hit h;
                ishit = objGroups->intersect(r, h, tmin);

                if (ishit)
                {
                    float t = h.getT();
#ifdef DEBUG
                    printf("t=%f, tmin=%f, tmax=%f\n", t, depth_min, depth_max);
#endif
                    if (t > depth_max || t < depth_min)
                        continue;

                    float GrayComponent = (t - depth_min) / (depth_max - depth_min);
                    GrayComponent = 1 - GrayComponent;
#ifdef DEBUG
                    printf("GrayComponent =%f\n", GrayComponent);
#endif
                    Vec3f color(GrayComponent, GrayComponent, GrayComponent);
                    depthtImg.SetPixel(i, j, color);
                }

            }

        depthtImg.SaveTGA(depth_file);
    }
}

void RenderNormal()
{
    if (normal_file == NULL )
    {
        fprintf(stderr, "The normal file is null, no need to render.\n");
    }
    else
    {
        Image normalImg(width, height);

        Camera * pCamera = parser->getCamera();
        Group * objGroups = parser->getGroup();

        int i;
        int j;
        float tmin = pCamera->getTMin();

        if (pCamera == NULL)
        {
            fprintf(stderr, "The camera is not defined in config file, so quit.");
            return;
        }

        for (j = 0; j < height; j++)
            for (i = 0; i < width; i++)
            {
                float u = (i + 0.5) / width;
                float v = (j + 0.5) / height;
                Vec2f p(u, v);
                Ray	r = pCamera->generateRay(p);

                bool ishit = false;
                Hit h;
                ishit = objGroups->intersect(r, h, tmin);

                if (ishit)
                {
                    Vec3f n = h.getNormal();

                    if (n.Length() != 1)
                        n.Normalize();

                    normalImg.SetPixel(i, j, n);
                }
            }

        normalImg.SaveTGA(normal_file);
    }
}

void RenderAll()
{
    RenderScene();
    RenderDepth();
    RenderNormal();
}

int main(int argc, char ** argv)
{

    parseArgs(argc, argv);
    //Now we start to parse the scene file
    parser = new SceneParser(input_file);

    //OpenGL ui
    if (guiMode)
    {
        Sphere::setTesselationSize(theta_steps, phi_steps);
        glutInit(&argc, argv);
        GLCanvas canvas;
        canvas.initialize(parser, RenderAll);
    }
    else
    {
        RenderAll();
    }

    delete parser;
    return 0;
}
