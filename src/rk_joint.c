/* RoKi - Robot Kinetics library
 * Copyright (C) 1998 Tomomichi Sugihara (Zhidao)
 *
 * rk_joint - joint structure
 */

#include <roki/rk_joint.h>

/* ********************************************************** */
/* CLASS: rkJoint
 * joint class
 * ********************************************************** */

RK_JOINT_COM_ARRAY;

rkJoint *rkJointAssign(rkJoint *j, rkJointCom *com)
{
  rkJointInit( j );
  if( ( j->prp = ( j->com = com )->_alloc() ) )
    j->com->_init( j->prp );
  rkJointNeutral( j );
  return j;
}

rkJoint *rkJointQueryAssign(rkJoint *j, char *str)
{
  register int i;

  for( i=0; rk_joint_com[i]; i++ )
    if( strcmp( rk_joint_com[i]->typestr, str ) == 0 )
      return rkJointAssign( j, rk_joint_com[i] );
  return NULL;
}

/* destroy a joint object. */
void rkJointDestroy(rkJoint *j)
{
  zFree( j->prp );
  rkJointInit( j );
}

/* neutralize joint displacement. */
void rkJointNeutral(rkJoint *j)
{
  double dis[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
  rkJointSetDis( j, dis );
}

/* check if joint displacement is neutral. */
bool rkJointIsNeutral(rkJoint *j)
{
  double dis[6];
  register int i;

  rkJointGetDis( j, dis );
  for( i=0; i<rkJointSize(j); i++ )
    if( !zIsTiny( dis[i] ) ) return false;
  return true;
}

/* clone a joint. */
rkJoint *rkJointClone(rkJoint *org, rkJoint *cln)
{
  rkMotor *morg, *mcln;

  if( !rkJointAssign( cln, org->com ) ) return NULL;
  rkJointCopyState( org, cln );
  if( ( morg = rkJointGetMotor( org ) ) && ( mcln = rkJointGetMotor( cln ) ) )
    rkMotorClone( morg, mcln );
  return cln;
}

/* copy joint state. */
rkJoint *rkJointCopyState(rkJoint *src, rkJoint *dst)
{
  double val[6];

  rkJointGetDis( src, val ); rkJointSetDis( dst, val );
  rkJointGetVel( src, val ); rkJointSetVel( dst, val );
  rkJointGetAcc( src, val ); rkJointSetAcc( dst, val );
  rkJointGetTrq( src, val ); rkJointSetTrq( dst, val );
  return dst;
}

/* increment motion rate due to joint rate. */
void rkJointIncRate(rkJoint *j, zVec3D *w, zVec6D *vel, zVec6D *acc)
{
  rkJointIncVel( j, vel );
  rkJointIncAccOnVel( j, w, acc );
  rkJointIncAcc( j, acc );
}

/* NOTE: The following macros and functions are for sharing
 * some operation codes. Do not use them in users programs. */
zVec3D *_rkJointAxisNull(void *prp, zFrame3D *f, zVec3D *a){
  return NULL;
}

zVec3D *_rkJointAxisZ(void *prp, zFrame3D *f, zVec3D *a){
  zVec3DCopy( &zFrame3DAtt(f)->e[zZ], a );
  return a;
}

/* joint torsion */

double rkJointRevolTorsionDis(zFrame3D *dev, zVec6D *t)
{
  zMat3D rm;
  zVec3D aa;
  double l, angle;

  zVec3DCreate( &aa, -zFrame3DAtt(dev)->e[2][1], zFrame3DAtt(dev)->e[2][0], 0 );
  l = sqrt( zSqr(aa.e[zX]) + zSqr(aa.e[zY]) );
  angle = atan2( l, zFrame3DAtt(dev)->e[2][2] );
  zIsTiny( angle ) ?
    zVec3DZero( &aa ) : zVec3DMulDRC( &aa, angle/l );
  zMulMat3DTVec3D( zFrame3DAtt(dev), &aa, zVec6DAng(t) );
  /* intermediate attitude */
  zMat3DFromAA( &rm, &aa );
  /* joint displacement */
  return 0.5 *
    ( zVec3DAngle( &rm.v[zX], &zFrame3DAtt(dev)->v[zX], &rm.v[zZ] )
    + zVec3DAngle( &rm.v[zY], &zFrame3DAtt(dev)->v[zY], &rm.v[zZ] ) );
}

double rkJointPrismTorsionDis(zFrame3D *dev, zVec6D *t)
{
  double q;

  zMulMat3DTVec3D( zFrame3DAtt(dev), zFrame3DPos(dev), zVec6DLin(t) );
  /* joint displacement */
  q = t->e[zZ];
  t->e[zZ] = 0;
  return q;
}

/* for ABI */
zMat6D *rkJointXformMat6D(zFrame3D *f, zMat6D *i, zMat6D *m)
{
  zMat3D tmpm, tmpm2;

  zRotMat3D( zFrame3DAtt(f), &i->e[0][0], &m->e[0][0] );
  zRotMat3D( zFrame3DAtt(f), &i->e[0][1], &m->e[0][1] );
  zRotMat3D( zFrame3DAtt(f), &i->e[1][1], &m->e[1][1] );

  zMulVec3DOuterProdMat3D( zFrame3DPos(f), &m->e[0][0], &tmpm );
  zMat3DT( &m->e[0][1], &tmpm2 );
  zMat3DAddDRC( &m->e[0][1], &tmpm );
  zMat3DT( &m->e[0][1], &m->e[1][0] );
  zMulVec3DOuterProdMat3D( zFrame3DPos(f), &m->e[1][0], &tmpm );
  zMat3DAddDRC( &m->e[1][1], &tmpm );
  zMulVec3DOuterProdMat3D( zFrame3DPos(f), &tmpm2, &tmpm );
  zMat3DT( &tmpm, &tmpm );
  zMat3DAddDRC( &m->e[1][1], &tmpm );
  return m;
}

void _rkJointUpdateWrench(rkJoint *j, zMat6D *i, zVec6D *b, zVec6D *acc)
{
  zMulMat6DVec6D( i, acc, rkJointWrench(j) );
  zVec6DAddDRC( rkJointWrench(j), b );
}

rkJoint *rkJointFromZTK(rkJoint *joint, rkMotorArray *motorarray, ZTK *ztk)
{
  return joint->com->_fromZTK( joint->prp, motorarray, ztk ) ? joint : NULL;
}
