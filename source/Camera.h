#ifndef CAMERA_H
#define CAMERA_H

#include "stdafx.h"

class Camera
{
public:
	Camera();
	~Camera();

	void setLens(float fovY, float aspect, float zn, float zf);
	void SetPosition(D3DXVECTOR3 lPos);

	void strafe(float d);
	void walk(float d);
	void setYPosition(float y);
	void pitch(float angle);
	void rotateY(float angle);
	void rebuildView();

	//
	D3DXVECTOR4 GetPosition()	const { return D3DXVECTOR4(mPosition, 1);};
	D3DXVECTOR4 GetLookAt()		const { return D3DXVECTOR4(mLook, 0) ;};
	D3DXVECTOR4 GetUp()			const { return D3DXVECTOR4(mUp, 0) ;};
	D3DXVECTOR4 GetRight()		const { return D3DXVECTOR4(mRight, 0) ;};

	D3DXMATRIX GetView()		const{ return mView; }
	D3DXMATRIX GetProj()		const{ return mProj;}

private:
	D3DXVECTOR3 mPosition;
	D3DXVECTOR3 mRight;
	D3DXVECTOR3 mUp;
	D3DXVECTOR3 mLook;

	D3DXMATRIX mView;
	D3DXMATRIX mProj;
};

Camera& GetCamera();

#endif