//--------------------------------------------------------------------------------------
// BasicCompute.fx
// Direct3D 11 Shader Model 5.0 Demo
// Copyright (c) Stefan Petersson, 2012
//--------------------------------------------------------------------------------------

#include "LightHelper.fx"
//#include "RayStruct.fx"
#define EPSILON 0.000001
#define SPHERE_COUNT 3
#define TRIANGLE_COUNT 4
#define LIGHT_COUNT 9



#define WHITE float3(1.0f, 1.0f, 1.0f)
#define BLACK float3(0.0f, 0.0f, 0.0f)

#define RED   float3(1.0f, 0.0f, 0.0f)
#define GREEN float3(0.0f, 1.0f, 0.0f)
#define BLUE float3(0.0f, 0.0f, 1.0f)


#pragma pack_matrix(row_major)

RWTexture2D<float4> output : register(u0);

struct SphereStruct
{
	float4	midPos;
	float3	color;
	float	radius;
	Material material;
};

struct TriangleStruct
{	
	float4  pos0;
	float4  pos1;
	float4  pos2;
	float4  color;			//float4 for padding reasons 
	Material material;
};

cbuffer EveryFrameBuffer : register(c0) 
{
	float4	 cameraPosition;
	float4x4 inverseProjection;
	float4x4 inverseView;
	float4 screenVariable;
	//float screenWidth;
	//float screenHeight;
	//float padding1;
	//float padding2;
}

cbuffer PrimitiveBuffer: register(c1)
{
	SphereStruct	Sphere[SPHERE_COUNT];
	TriangleStruct	Triangle[TRIANGLE_COUNT];
	float4			countVariable;
}

cbuffer LightBuffer : register(c2) 
{
	float light_count;
	float3 ambientLight;
	PointLightData PointLight[LIGHT_COUNT];
}

Ray createRay(int x, int y)
{
	Ray l_ray;
	l_ray.origin = cameraPosition;
 
	double normalized_x = ((x / screenVariable.x) - 0.5) * 2;
	double normalized_y = (1 - (y / screenVariable.y) - 0.5) * 2;

//	double normalized_x = ((x / screenWidth) - 0.5) * 2;		// Other variable name
//	double normalized_y = (1 - (y / screenHeight) - 0.5) * 2;	// Other variable name
 
	float4 imagePoint = mul(float4(normalized_x, normalized_y, 1, 1), inverseProjection);
	imagePoint /= imagePoint.w;
 
	imagePoint = mul(imagePoint, inverseView);
 
	l_ray.direction = imagePoint - l_ray.origin;
	l_ray.direction = normalize(l_ray.direction);
//	l_ray.direction.z = 0.0f;						// Used to test code
	l_ray.color = float4(0.0f, 0.0f, 0.0f, 1.0f);

	l_ray.lastWasHit = true;

	return l_ray;
}

float3 TriangleNormalCounterClockwise(int index)
{
	float3 e1, e2;  //Edge1, Edge2
 
	//Find vectors for two edges sharing V0
	e1 = Triangle[index].pos1.xyz - Triangle[index].pos0.xyz;
	e2 = Triangle[index].pos2.xyz - Triangle[index].pos0.xyz;	
	
	float3 normal = cross(e1, e2);
	return normalize(normal);	
}
/*
void CheckIfRayHits(in Ray p_ray, out int p_hitSphere, out int p_hitTriangle, out int p_closestSphereIndex, out int p_closestTriangleIndex, out float p_distanceToClosestSphere, out int p_distanceToClosestTriangle)
{
	// Check if this is lit by the light. Means that we check if this intersects anything else. If not it is lit by the light.
	float temp = 0.0f;
	float lowest = 0.0f;
	
	float l_sphereDistance = 0.0f;
	int l_sphereIndex = 0;

	float l_triangleDistance = 0.0f;
	int l_triangleIndex = 0;

	p_hitSphere = -1;
	p_hitTriangle = -1;

	for(int i = 0; i < countVariable.x; i++)				// Go through all spheres
	{
		temp = SphereIntersect(p_ray, i);				// Get distance to current sphere, return 0.0 if it does not intersect
		if(temp > 0.0f)
		{
			if(lowest == 0.0)
				lowest = temp;
			if(temp != 0.0f)	
			{
				p_hitSphere = 1;
				if(temp < lowest) //|| l_sphereDistance == 0.0f)	// Sets the new value to l_sphereHitDistance if it's lower than before or if l_sphereHitDistance equals 0.0f (first attempt)
				{
					lowest = temp;
					l_sphereIndex = i;
				}
			}
		}
	}
	l_sphereDistance = lowest;

	lowest = 0.0;
	for(int i = 0; i < countVariable.y; i++)
	{
		temp = TriangleIntersect(p_ray, i);
		if(temp > 0.0f)
		{
			if(lowest == 0.0f)
				lowest = temp;
			if(temp != 0.0f)	
			{
				p_hitTriangle = 1;
				if(temp < lowest)
				{
					lowest = temp;
					l_triangleIndex = i;
				}
			}
		}
	}
	l_triangleDistance = lowest;
	
	p_closestSphereIndex =  l_sphereIndex;
	p_closestTriangleIndex = l_triangleIndex;
	p_distanceToClosestSphere = l_sphereDistance;
	p_distanceToClosestTriangle = l_triangleDistance;
}
*/

interface IntersectInterface
{
	float Intersect(in Ray p_ray, in int p_index);
};

class SphereIntersect : IntersectInterface
{
	float Intersect(in Ray p_ray, in int index)
	{		
		//return 0.0f; //Skip spheres
		float4 l_distance = p_ray.origin - Sphere[index].midPos;
		float a, b, t, t1, t2;

		b = dot(p_ray.direction, l_distance);
		a = dot(l_distance, l_distance ) - (Sphere[index].radius * Sphere[index].radius);
		if(b * b - a >= 0)
		{
			t = sqrt(b * b - a);
			t1 = -b + t;  
			t2 = -b - t;
			if( t1 > 0.0f || t2 > 0.0f) // checks so atleast one of the values are infront of the camera
			{			
				if(t1 < t2 && t1 > 0)
					return t1;
				else if( t2 > 0)
					return t2;
				//return t1 < t2 ? t1 : t2;		//Returns the smallest number
			}
		}
		return 0.0f;		// ifwe didn't hit	
	}
};

class TriangleIntersect : IntersectInterface
{
	float Intersect(Ray p_ray, int index)                         
	{
		//return 0.0f;
		float3 e1, e2;  //Edge1, Edge2
		float det, inv_det, u, v;
		float t;
 
		//Find vectors for two edges sharing V0
		e1 = Triangle[index].pos1.xyz - Triangle[index].pos0.xyz;
		e2 = Triangle[index].pos2.xyz - Triangle[index].pos0.xyz;	
	
		//Begin calculating determinant - also used to calculate u parameter
		float3 P = cross(p_ray.direction.xyz, e2);
	
		//if determinant is near zero, ray lies in plane of triangle
		det = dot(e1, P);
	
		//NOT CULLING
		if(det > -EPSILON && det < EPSILON) 
			return 0.0f;
	
		inv_det = 1.0f / det;

		//calculate distance from V0 to ray origin
		float3 T = p_ray.origin.xyz - Triangle[index].pos0.xyz;

		//Calculate u parameter and test bound
		u = dot(T, P) * inv_det;
	
		//The intersection lies outside of the triangle
		if(u < 0.0f || u > 1.0f) 
			return 0.0f;

		//Prepare to test v parameter
		float3 Q = cross(T, e1);

		//Calculate V parameter and test bound
		v = dot(p_ray.direction.xyz, Q) * inv_det;
	
		//The intersection lies outside of the triangle
		if(v < 0.0f || u + v  > 1.0f) 
			return 0.0f;

		t = dot(e2, Q) * inv_det;

		if(t > EPSILON) 
			return t; //ray intersection

		// No hit, no win
		return 0.0f;
	}
};

void GetClosestPrimitive(in Ray p_ray, in IntersectInterface p_intersect, int p_amount, out int p_hitPrimitive, out int p_closestPrimitiveIndex, out float p_distanceToClosestPrimitive)
{	
	p_hitPrimitive = -1;
	
	int l_primitiveIndex = 0;
	//int l_primitiveIndex = 0;
	
	float temp = 0.0f;
	float lowest = 0.0f;

	for(int i = 0; i < p_amount; i++)				// Go through all spheres
	{
		temp = p_intersect.Intersect(p_ray, i);				// Get distance to current sphere, return 0.0 if it does not intersect
		if(temp != 0.0f && temp > 0.0f)	
		{
			p_hitPrimitive = 1;
			if(temp < lowest || lowest == 0.0f) // Sets the new value to l_sphereHitDistance if it's lower than before or if l_sphereHitDistance equals 0.0f (first attempt)
			{
				lowest = temp;
				l_primitiveIndex = i;
			}
		}
	}
	p_closestPrimitiveIndex =  l_primitiveIndex;
	p_distanceToClosestPrimitive = lowest;
}

#define THING i

Ray Phong(inout Ray p_ray) // first jump == 0
{	
	// Variables used by all intersections
	float4 l_tempColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 l_collidePos, l_collideNormal;

	// Sphere specific variable
	float l_sphereHitDistance	= 0.0f;
	int l_sphereindex = 0;

	float l_triangleHitDistance = 0.0f;
	int l_triangleindex = 0;

	p_ray.lastWasHit = false;

	SphereIntersect sphereIntersect;
	TriangleIntersect triangleIntersect;
	
	int sp1, tr1;
	GetClosestPrimitive(p_ray, sphereIntersect, countVariable.x, sp1, l_sphereindex, l_sphereHitDistance); // Spheres
		
	GetClosestPrimitive(p_ray, triangleIntersect, countVariable.y, tr1, l_triangleindex, l_triangleHitDistance); // Triangles
	

	////////////////////////////// Checks to se if any triangle or sphere was hit at all
	if(l_triangleHitDistance == 0.0f && l_sphereHitDistance == 0.0f)
	{
		return p_ray; // It hit nothing, return direct
	}

	////////////////////////////// Checks which primitive is closest

	//	if l_sphereHitDistance was NOT equal to zero 
	//	AND atleast one of the following three is correct:
	//	l_triangleHitDistance is equal to 0, means there was no hit on triangle at all
	//	l_triangleHitDistance is bigger than 0, what does this mean?
	//	l_sphereHitDistance is smaller than l_triangleHitDistance. Means that both a triangle and a sphere was hit, but that sphere was closer.
	//if(sp1 != -1 && (tr1 == -1 || l_triangleHitDistance < 0.0f || l_sphereHitDistance < l_triangleHitDistance))
	if(0.0f != l_sphereHitDistance && (l_triangleHitDistance == 0.0f || l_triangleHitDistance < 0.0f || l_sphereHitDistance < l_triangleHitDistance))
	{			
		p_ray.lastWasHit = true;

		// Reflect code
		l_collidePos = p_ray.origin + (l_sphereHitDistance - 0.0001) * p_ray.direction;
		l_collideNormal = -normalize(l_collidePos - Sphere[l_sphereindex].midPos); // Reverse normal
		
		p_ray.origin = l_collidePos; // new origin for next jump
		p_ray.direction = float4(reflect(p_ray.direction.xyz, l_collideNormal), 0.0f); // new direction for next jump
				
		// Vector from light source
		Ray l_lightSourceRay;
		l_lightSourceRay.origin = PointLight[0].position;
		l_lightSourceRay.direction = normalize(p_ray.origin - PointLight[0].position);
		
		int l_closestSphereIndex, l_closestTriangleIndex;
		float l_distanceToClosestSphere, l_distanceToClosestTriangle = 0.0f;
		int sp = -1;
		int tr = -1;

		GetClosestPrimitive(l_lightSourceRay, sphereIntersect, countVariable.x, sp, l_closestSphereIndex, l_distanceToClosestSphere);
		
		GetClosestPrimitive(l_lightSourceRay, triangleIntersect, countVariable.y, tr, l_closestTriangleIndex, l_distanceToClosestTriangle);
		
		for(int i = 0; i < LIGHT_COUNT; i++)
		{
		float4 a = 0.0f * float4(Sphere[l_sphereindex].color, 1.0f) * CalcLight(p_ray, PointLight[THING], p_ray.origin, l_collideNormal, Sphere[l_sphereindex].material, float4(ambientLight, 1.0f)); 
		if(sp != -1 && tr != -1) // Both a triangle and a sphere has been hit
		{
			if(l_distanceToClosestSphere < l_distanceToClosestTriangle ) // A sphere is the closest
			{
				if(l_sphereindex == l_closestSphereIndex)	// The sphere I am at is the closest
				{
					// Render sphere
					l_tempColor = float4(Sphere[l_sphereindex].color, 1.0f) * CalcLight(p_ray, PointLight[THING], p_ray.origin, l_collideNormal, Sphere[l_sphereindex].material, float4(ambientLight, 1.0f)); 
				}
				else //Another sphere was closer
					l_tempColor = a;		
			}
			else // A random triangle was closer
				l_tempColor = a;
		}
		else if(sp != -1 && tr == -1) // Only a sphere was hit
		{
			if(l_sphereindex == l_closestSphereIndex)	// The sphere I am at is the closest
			{
				// Render sphere
					l_tempColor = float4(Sphere[l_sphereindex].color, 1.0f) * CalcLight(p_ray, PointLight[THING], p_ray.origin, l_collideNormal, Sphere[l_sphereindex].material, float4(ambientLight, 1.0f)); 
			}
			else // Another sphere was closer
				l_tempColor = a;		
		}
		}
	 }

	//	if l_sphereHitDistance was NOT equal to zero 
	//	AND that atleast one of the following is correct
	//	that l_triangleHitDistance is greater than 0	
	//	l_sphereHitDistance is equal to 0.0, means no hit
	//	l_triangleHitDistance is smaller than l_sphereHitDistance. Means that both a triangle and a sphere was hit, but that triangle was closer.
	else if(l_triangleHitDistance != 0.0f  && ( l_sphereHitDistance == 0.0f  || l_sphereHitDistance == 0.0f  || l_triangleHitDistance < l_sphereHitDistance))
	{	
		p_ray.lastWasHit = true;

		
		// Make reflect code here
		l_collidePos = p_ray.origin + (l_triangleHitDistance - 0.0001) * p_ray.direction;
		l_collideNormal = float4(TriangleNormalCounterClockwise(l_triangleindex), 1.0f);

		p_ray.origin = l_collidePos;
		p_ray.direction = float4(reflect(p_ray.direction.xyz, l_collideNormal), 0.0f);


		// Vector from light source
		Ray l_lightSourceRay;
		l_lightSourceRay.origin = PointLight[0].position;
		l_lightSourceRay.direction = normalize(p_ray.origin - PointLight[0].position);
		
		int l_closestSphereIndex, l_closestTriangleIndex;
		float l_distanceToClosestSphere, l_distanceToClosestTriangle = 0.0f;
		int sp = -1;
		int tr = -1;

		GetClosestPrimitive(l_lightSourceRay, sphereIntersect, countVariable.x, sp, l_closestSphereIndex, l_distanceToClosestSphere);
		
		GetClosestPrimitive(l_lightSourceRay, triangleIntersect, countVariable.y, tr, l_closestTriangleIndex, l_distanceToClosestTriangle);
		for(int i = 0; i < LIGHT_COUNT; i++)
		{
		float4 a = 0.0f * Triangle[l_triangleindex].color * CalcLight(p_ray, PointLight[THING], p_ray.origin, l_collideNormal, Triangle[l_triangleindex].material, float4(ambientLight, 1.0f)); 
		if(sp != -1 && tr != -1) // Both a triangle and a sphere has been hit
		{
			if(l_distanceToClosestTriangle < l_distanceToClosestSphere) // Triangle is closest
			{
				if(l_triangleindex == l_closestTriangleIndex)	// The triangle I am at is  the closest
				{
					// Render triangle
					l_tempColor = Triangle[l_triangleindex].color * CalcLight(p_ray, PointLight[THING], p_ray.origin, l_collideNormal, Triangle[l_triangleindex].material, float4(ambientLight, 1.0f)); 
				}
				else //Another triangle was closer
				{
					l_tempColor = a;		
				}
			}
			else // A random Sphere was closer
			{
				l_tempColor = a;
			}
		}
		else if(sp == -1 && tr != -1) // Only a triangle was hit
		{
			if(l_triangleindex == l_closestTriangleIndex)	// The triangle I am at is the closest
			{
				// Render triangle
					l_tempColor = Triangle[l_triangleindex].color * CalcLight(p_ray, PointLight[THING], p_ray.origin, l_collideNormal, Triangle[l_triangleindex].material, float4(ambientLight, 1.0f)); 
			}
			else // Another triangle was closer
				l_tempColor = a;		
		}
		}
	}	
	else
	{
		// This is a debug place, should never happen.
		l_tempColor = float4(1.0f, 0.55f, 0.0f, 1.0f); //ORANGES
	}

	p_ray.color = l_tempColor;

	return p_ray;
}

#define max_number_of_bounces 3
[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID)
{
	float4 l_finalColor = float4(0,0,0,1);
	Ray l_ray = createRay(threadID.x, threadID.y);
		

	for(int i = 0; i < max_number_of_bounces; i++)	
	{
		l_ray = Phong(l_ray);	
	}
	
	output[threadID.xy] = l_ray.color;
//output[threadID.xy] = l_ray.direction; //Debug thingy sak f�r att se om rays faktiskt blir n�t	
}


/*
	Fixa s� att ljuset r�r sig.
	Fixa till s� att det ser bra ut.
	V�ldigt skrikiga f�rger
	Fixa flimmret som �r
	Dela upp saker i funktioner
	Dynamiska buffrar
	Ladda in objekt
	Material
	Ljus som studsar? :3

	Tankar
		Octatree
		Hashtable som anv�nder pekare som nyckel
		Boundingbox f�r varje objekt f�r snabbare koll.
		Skapa egna ComputeShaders

*/