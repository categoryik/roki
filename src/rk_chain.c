/* RoKi - Robot Kinetics library
 * Copyright (C) 1998 Tomomichi Sugihara (Zhidao)
 *
 * rk_chain - kinematic chain, kinematics and dynamics
 */

#include <roki/rk_chain.h>

/* ********************************************************** */
/* CLASS: rkChain
 * kinematic chain class
 * ********************************************************** */

/* initialize a kinematic chain. */
void rkChainInit(rkChain *c)
{
  zNameSetPtr( c, NULL );
  zArrayInit( &c->link );
  rkChainSetShape( c, NULL );
  rkChainSetMotor( c, NULL );
  rkChainSetMass( c, 0 );
  rkChainSetWldCOM( c, ZVEC3DZERO );
  rkChainSetCOMVel( c, ZVEC3DZERO );
  rkChainSetCOMAcc( c, ZVEC3DZERO );
}

/* destroy a kinematic chain. */
void rkChainDestroy(rkChain *c)
{
  register int i;

  if( !c ) return;
  zNameFree(c);
  for( i=0; i<rkChainLinkNum(c); i++ )
    rkLinkDestroy( rkChainLink(c,i) );
  zArrayFree( &c->link );
  zMShape3DDestroy( rkChainShape(c) );
  zFree( rkChainShape(c) );
  if( rkChainMotor(c) ){
    zArrayFree( rkChainMotor(c) );
    zFree( rkChainMotor(c) );
  }
  rkChainInit( c );
}

/* clone a kinematic chain. */
rkChain *rkChainClone(rkChain *org, rkChain *cln)
{
  char name[BUFSIZ];
  int i;

  if( !cln || !org ){
    ZRUNERROR( RK_WARN_CHAIN_NULL );
    return NULL;
  }
  if( rkChainLinkNum(org) == 0 ){
    ZRUNERROR( RK_WARN_CHAIN_EMPTY );
    return NULL;
  }
  rkChainInit( cln );
  sprintf( name, "%s_clone", zName(org) );
  if( !zNameSet( cln, name ) ||
      ( rkChainShape(org) && !( rkChainShape(cln) = zMShape3DClone( rkChainShape(org) ) ) ) ||
      ( rkChainMotor(org) && !( rkChainMotor(cln) = rkMotorArrayClone( rkChainMotor(org) ) ) ) ){
    ZALLOCERROR();
    return NULL;
  }
  zArrayAlloc( &cln->link, rkLink, rkChainLinkNum(org) );
  if( rkChainLinkNum(cln) != rkChainLinkNum(org) ) return NULL;
  for( i=0; i<rkChainLinkNum(cln); i++ )
    if( !rkLinkClone( rkChainLink(org,i), rkChainLink(cln,i), rkChainShape(org), rkChainShape(cln) ) )
      return NULL;
  rkChainSetMass( cln, rkChainMass(org) );
  rkChainCopyState( org, cln );
  return cln;
}

/* copy state of a kinematic chain. */
rkChain *rkChainCopyState(rkChain *src, rkChain *dst)
{
  register int i;

  for( i=0; i<rkChainLinkNum(src); i++ )
    rkLinkCopyState( rkChainLink(src,i), rkChainLink(dst,i) );
  zVec3DCopy( rkChainWldCOM(src), rkChainWldCOM(dst) );
  zVec3DCopy( rkChainCOMVel(src), rkChainCOMVel(dst) );
  zVec3DCopy( rkChainCOMAcc(src), rkChainCOMAcc(dst) );
  return dst;
}

/* count the total number of joint of a kinematic chain. */
int rkChainJointSize(rkChain *c)
{
  register int i, size;

  for( size=0, i=0; i<rkChainLinkNum(c); i++ )
    size += rkChainLinkJointSize(c,i);
  return size;
}

/* create default joint index of a kinematic chain. */
zIndex rkChainCreateDefaultJointIndex(rkChain *c)
{
  register int i, count;
  zIndex index;

  for( count=0, i=0; i<rkChainLinkNum(c); i++ )
    if( rkChainLinkJointSize(c,i) > 0 ) count++;
  if( !( index = zIndexCreate( count ) ) ){
    ZALLOCERROR();
    return NULL;
  }
  for( count=0, i=0; i<rkChainLinkNum(c); i++ )
    if( rkChainLinkJointSize(c,i) > 0 ){
      zIndexSetElemNC( index, count, i );
      count++;
    }
  return index;
}

/* count the total joint size indicated in a kinematic chain. */
int rkChainJointIndexSize(rkChain *c, zIndex idx)
{
  register int i, size;

  for( size=0, i=0; i<zArraySize(idx); i++ )
    size += rkChainLinkJointSize(c,zIndexElemNC(idx,i));
  return size;
}

/* set joint displacements of a kinematic chain. */
void rkChainSetJointDis(rkChain *c, zIndex idx, zVec dis)
{
  register int i;
  double *dp;

  for( dp=zVecBuf(dis), i=0; i<zArraySize(idx); i++ ){
    rkChainLinkJointSetDis( c, zIndexElemNC(idx,i), dp );
    dp += rkChainLinkJointSize(c,zIndexElemNC(idx,i));
  }
}

/* continuously update joint displacements of a kinematic chain. */
void rkChainSetJointDisCNT(rkChain *c, zIndex idx, zVec dis, double dt)
{
  register int i;
  double *dp;

  for( dp=zVecBuf(dis), i=0; i<zArraySize(idx); i++ ){
    rkChainLinkJointSetDisCNT( c, zIndexElemNC(idx,i), dp, dt );
    dp += rkChainLinkJointSize(c,zIndexElemNC(idx,i));
  }
}

/* set joint velocities of a kinematic chain. */
void rkChainSetJointVel(rkChain *c, zIndex idx, zVec vel)
{
  register int i;
  double *vp;

  for( vp=zVecBuf(vel), i=0; i<zArraySize(idx); i++ ){
    rkJointSetVel( rkChainLinkJoint(c,zIndexElemNC(idx,i)), vp );
    vp += rkChainLinkJointSize(c,zIndexElemNC(idx,i));
  }
}

/* set joint accelerations of a kinematic chain. */
void rkChainSetJointAcc(rkChain *c, zIndex idx, zVec acc)
{
  register int i;
  double *vp;

  for( vp=zVecBuf(acc), i=0; i<zArraySize(idx); i++ ){
    rkJointSetAcc( rkChainLinkJoint(c,zIndexElemNC(idx,i)), vp );
    vp += rkChainLinkJointSize(c,zIndexElemNC(idx,i));
  }
}

/* set joint velocities and accelerations of a kinematic chain. */
void rkChainSetJointRate(rkChain *c, zIndex idx, zVec vel, zVec acc)
{
  register int i;
  double *vp, *ap;

  for( vp=zVecBuf(vel), ap=zVecBuf(acc), i=0; i<zArraySize(idx); i++ ){
    rkJointSetVel( rkChainLinkJoint(c,zIndexElemNC(idx,i)), vp );
    rkJointSetAcc( rkChainLinkJoint(c,zIndexElemNC(idx,i)), ap );
    vp += rkChainLinkJointSize(c,zIndexElemNC(idx,i));
    ap += rkChainLinkJointSize(c,zIndexElemNC(idx,i));
  }
}

/* get joint displacements of a kinematic chain. */
zVec rkChainGetJointDis(rkChain *c, zIndex idx, zVec dis)
{
  register int i;
  double *dp;

  for( dp=zVecBuf(dis), i=0; i<zArraySize(idx); i++ ){
    rkChainLinkJointGetDis( c, zIndexElemNC(idx,i), dp );
    dp += rkChainLinkJointSize(c,zIndexElemNC(idx,i));
  }
  return dis;
}

/* get joint velocities of a kinematic chain. */
zVec rkChainGetJointVel(rkChain *c, zIndex idx, zVec vel)
{
  register int i;
  double *dp;

  for( dp=zVecBuf(vel), i=0; i<zArraySize(idx); i++ ){
    rkChainLinkJointGetVel( c, zIndexElemNC(idx,i), dp );
    dp += rkChainLinkJointSize(c,zIndexElemNC(idx,i));
  }
  return vel;
}

/* get joint accelerations of a kinematic chain. */
zVec rkChainGetJointAcc(rkChain *c, zIndex idx, zVec acc)
{
  register int i;
  double *dp;

  for( dp=zVecBuf(acc), i=0; i<zArraySize(idx); i++ ){
    rkChainLinkJointGetAcc( c, zIndexElemNC(idx,i), dp );
    dp += rkChainLinkJointSize(c,zIndexElemNC(idx,i));
  }
  return acc;
}

/* set all joint displacements of a kinematic chain. */
void rkChainSetJointDisAll(rkChain *c, zVec dis)
{
  register int i;

  if( dis ){
    for( i=0; i<rkChainLinkNum(c); i++ )
      if( rkChainLinkOffset(c,i) >= 0 )
        rkChainLinkJointSetDis( c, i, &zVecElemNC(dis,rkChainLinkOffset(c,i)) );
  } else{
    for( i=0; i<rkChainLinkNum(c); i++ )
      if( rkChainLinkOffset(c,i) >= 0 )
        rkChainLinkJointSetDis( c, i, ZVEC6DZERO->e );
  }
}

/* concatenate all joint displacements of a kinematic chain. */
void rkChainCatJointDisAll(rkChain *c, zVec dis, double k, zVec v)
{
  register int i;

  for( i=0; i<rkChainLinkNum(c); i++ )
    if( rkChainLinkOffset(c,i) >= 0 )
      rkJointCatDis( rkChainLinkJoint(c,i), &zVecElemNC(dis,rkChainLinkOffset(c,i)), k, &zVecElemNC(v,rkChainLinkOffset(c,i)) );
}

/* subtract all joint displacements of a kinematic chain. */
void rkChainSubJointDisAll(rkChain *c, zVec dis, zVec sdis)
{
  register int i;

  for( i=0; i<rkChainLinkNum(c); i++ )
    if( rkChainLinkOffset(c,i) >= 0 )
      rkJointSubDis( rkChainLinkJoint(c,i), &zVecElemNC(dis,rkChainLinkOffset(c,i)), &zVecElemNC(sdis,rkChainLinkOffset(c,i)) );
}

/* continuously update all joint displacements of a kinematic chain. */
void rkChainSetJointDisCNTAll(rkChain *c, zVec dis, double dt)
{
  register int i;

  for( i=0; i<rkChainLinkNum(c); i++ )
    if( rkChainLinkOffset(c,i) >= 0 )
      rkChainLinkJointSetDisCNT( c, i, &zVecElemNC(dis,rkChainLinkOffset(c,i)), dt );
}

/* set all joint velocities of a kinematic chain. */
void rkChainSetJointVelAll(rkChain *c, zVec vel)
{
  register int i;

  if( vel ){
    for( i=0; i<rkChainLinkNum(c); i++ )
      if( rkChainLinkOffset(c,i) >= 0 )
        rkJointSetVel( rkChainLinkJoint(c,i), &zVecElemNC(vel,rkChainLinkOffset(c,i)) );
  } else{
    for( i=0; i<rkChainLinkNum(c); i++ )
      if( rkChainLinkOffset(c,i) >= 0 )
        rkJointSetVel( rkChainLinkJoint(c,i), ZVEC6DZERO->e );
  }
}

/* set all joint accelerations of a kinematic chain. */
void rkChainSetJointAccAll(rkChain *c, zVec acc)
{
  register int i;

  if( acc ){
    for( i=0; i<rkChainLinkNum(c); i++ )
      if( rkChainLinkOffset(c,i) >= 0 )
        rkJointSetAcc( rkChainLinkJoint(c,i), &zVecElemNC(acc,rkChainLinkOffset(c,i)) );
  } else{
    for( i=0; i<rkChainLinkNum(c); i++ )
      if( rkChainLinkOffset(c,i) >= 0 )
        rkJointSetAcc( rkChainLinkJoint(c,i), ZVEC6DZERO->e );
  }
}

/* set all joint velocities and accelerations of a kinematic chain. */
void rkChainSetJointRateAll(rkChain *c, zVec vel, zVec acc)
{
  register int i;

  for( i=0; i<rkChainLinkNum(c); i++ )
    if( rkChainLinkOffset(c,i) >= 0 ){
      rkJointSetVel( rkChainLinkJoint(c,i), &zVecElemNC(vel,rkChainLinkOffset(c,i)) );
      rkJointSetAcc( rkChainLinkJoint(c,i), &zVecElemNC(acc,rkChainLinkOffset(c,i)) );
    }
}

/* get all joint displacements of a kinematic chain. */
zVec rkChainGetJointDisAll(rkChain *c, zVec dis)
{
  register int i;

  for( i=0; i<rkChainLinkNum(c); i++ )
    if( rkChainLinkOffset(c,i) >= 0 )
      rkChainLinkJointGetDis( c, i, &zVecElemNC(dis,rkChainLinkOffset(c,i)) );
  return dis;
}

/* get all joint velocities of a kinematic chain. */
zVec rkChainGetJointVelAll(rkChain *c, zVec vel)
{
  register int i;

  for( i=0; i<rkChainLinkNum(c); i++ )
    if( rkChainLinkOffset(c,i) >= 0 )
      rkChainLinkJointGetVel( c, i, &zVecElemNC(vel,rkChainLinkOffset(c,i)) );
  return vel;
}

/* get all joint accelerations of a kinematic chain. */
zVec rkChainGetJointAccAll(rkChain *c, zVec acc)
{
  register int i;

  for( i=0; i<rkChainLinkNum(c); i++ )
    if( rkChainLinkOffset(c,i) >= 0 )
      rkChainLinkJointGetAcc( c, i, &zVecElemNC(acc,rkChainLinkOffset(c,i)) );
  return acc;
}

/* get all joint torques of a kinematic chain. */
zVec rkChainGetJointTrqAll(rkChain *c, zVec trq)
{
  register int i;

  for( i=0; i<rkChainLinkNum(c); i++ )
    if( rkChainLinkOffset(c,i) >= 0 )
      rkChainLinkJointGetTrq( c, i, &zVecElemNC(trq,rkChainLinkOffset(c,i)) );
  return trq;
}

/* get all link configurations of a kinematic chain. */
zVec rkChainGetConf(rkChain *chain, zVec conf)
{
  register int i;

  for( i=0; i<rkChainLinkNum(chain); i++ )
    zFrame3DToArrayAA( rkChainLinkWldFrame(chain,i), &zVecElemNC(conf,i*6) );
  return conf;
}

/* set all link configurations of a kinematic chain. */
void rkChainSetConf(rkChain *chain, zVec conf)
{
  register int i;

  for( i=0; i<rkChainLinkNum(chain); i++ )
    zArrayToFrame3DAA( &zVecElemNC(conf,i*6), rkChainLinkWldFrame(chain,i) );
  rkLinkConfToJointDis( rkChainRoot(chain) );
}

/* set all joint motor trq of a kinematic chain. */
void rkChainSetMotorInputAll(rkChain *c, zVec trq)
{
  register int i;

  for( i=0; i<rkChainLinkNum(c); i++ )
    if( rkChainLinkOffset(c,i) >= 0 ){
      rkChainLinkJointMotorSetInput(c,i,&zVecElemNC(trq,rkChainLinkOffset(c,i)));
    }
}

/* direction vector of gravity with respect to the body frame of a kinematic chain. */
zVec3D *rkChainGravityDir(rkChain *c, zVec3D *v)
{
  return zMat3DRow( rkChainRootAtt(c), 2, v );
}

/* update link frames of a kinematic chain via forward kinematics. */
void rkChainUpdateFK(rkChain *c)
{
  rkChainUpdateFrame( c );
  rkChainUpdateCOM( c );
}

/* solve forward kinematics of a kinematic chain. */
void rkChainFK(rkChain *c, zVec dis)
{
  rkChainSetJointDisAll( c, dis );
  rkChainUpdateFK( c );
}

/* update link states and joint torques of a kinematic chain via inverse dynamics. */
void rkChainUpdateID(rkChain *c)
{
  rkChainUpdateRate( c );
  rkChainUpdateWrench( c );
  rkChainUpdateCOMVel( c );
  rkChainUpdateCOMAcc( c );
}

/* solve inverse dynamics of a kinematic chain. */
void rkChainID(rkChain *c, zVec vel, zVec acc)
{
  rkChainSetJointRateAll( c, vel, acc );
  rkChainUpdateID( c );
}

/* continuously update joint displacements of a kinematic chain over a time step. */
void rkChainFKCNT(rkChain *c, zVec dis, double dt)
{
  rkChainSetJointDisCNTAll( c, dis, dt );
  rkChainUpdateFK( c );
  rkChainUpdateID( c );
}

/* link acceleration at zero joint acceleration. */
zVec6D *rkChainLinkZeroAcc(rkChain *chain, int id, zVec3D *p, zVec6D *a0)
{
  zVec3D tmp;

  rkChainSetJointAccAll( chain, NULL );
  rkChainUpdateRate( chain );
  rkChainLinkPointAcc( chain, id, p, &tmp );
  _zMulMat3DVec3D( rkChainLinkWldAtt(chain,id), &tmp, zVec6DLin(a0) );
  _zMulMat3DVec3D( rkChainLinkWldAtt(chain,id), rkChainLinkAngAcc(chain,id), zVec6DAng(a0) );
  return a0;
}

/* total mass of a kinematic chain. */
double rkChainCalcMass(rkChain *chain)
{
  register int i;

  rkChainSetMass( chain, 0.0 );
  for( i=0; i<rkChainLinkNum(chain); i++ )
    rkChainMass(chain) += rkChainLinkMass(chain,i);
  return rkChainMass(chain);
}

/* the center of mass of a kinematic chain with respect to the total/world frame. */
zVec3D *rkChainUpdateCOM(rkChain *c)
{
  register int i;

  rkChainSetWldCOM( c, ZVEC3DZERO );
  for( i=0; i<rkChainLinkNum(c); i++ )
    zVec3DCatDRC( rkChainWldCOM(c),
      rkChainLinkMass(c,i), rkChainLinkWldCOM(c,i) );
  return zVec3DDivDRC( rkChainWldCOM(c), rkChainMass(c) );
}

/* velocity of the center of mass of a kinematic chain with respect to the world frame. */
zVec3D *rkChainUpdateCOMVel(rkChain *c)
{
  register int i;
  zVec3D v;

  rkChainSetCOMVel( c, ZVEC3DZERO );
  for( i=0; i<rkChainLinkNum(c); i++ ){
    /* COM velocity of link is with respect to the local frame,
       while COM velocity of a kinematic chain is with respect to
       the world frame. */
    zMulMat3DVec3D( rkChainLinkWldAtt(c,i), rkChainLinkCOMVel(c,i), &v );
    zVec3DCatDRC( rkChainCOMVel(c), rkChainLinkMass(c,i)/rkChainMass(c), &v );
  }
  return rkChainCOMVel(c);
}

/* acceleration of the center of mass of a kinematic chain with respect to the world frame. */
zVec3D *rkChainUpdateCOMAcc(rkChain *c)
{
  register int i;
  zVec3D a;

  rkChainSetCOMAcc( c, ZVEC3DZERO );
  for( i=0; i<rkChainLinkNum(c); i++ ){
    /* COM acceleration of a link is with respect to the local frame,
       while COM acceleration of a kinematic chain is with respect to
       the world frame. */
    zMulMat3DVec3D( rkChainLinkWldAtt(c,i), rkChainLinkCOMAcc(c,i), &a );
    zVec3DCatDRC( rkChainCOMAcc(c), rkChainLinkMass(c,i)/rkChainMass(c), &a );
  }
  return rkChainCOMAcc(c);
}

/* Zero Moment Point of a kinematic chain. */
zVec3D *rkChainZMP(rkChain *c, double z, zVec3D *zmp)
{
  zVec3D dz;
  double f;

  rkChainGravityDir( c, &dz );
  if( zIsTiny( ( f = zVec3DInnerProd( &dz, rkChainRootForce(c) ) ) ) )
    return NULL; /* ZMP does not exist (floating). */
  zVec3DOuterProd( &dz, rkChainRootTorque(c), zmp );
  zVec3DCatDRC( zmp, z-rkChainRootPos(c)->e[zZ], rkChainRootForce(c) );
  zVec3DDivDRC( zmp, f );
  return zXform3DDRC( rkChainRootFrame(c), zmp );
}

/* net torque around vertical axis exerted to a kinematic chain. */
double rkChainYawTorque(rkChain *c)
{
  zVec3D dz;

  rkChainGravityDir( c, &dz );
  return zVec3DInnerProd(rkChainRootTorque(c),rkChainRootForce(c)) / zVec3DInnerProd(rkChainRootTorque(c),&dz);
}

/* angular momentum of a kinematic chain. */
zVec3D *rkChainAM(rkChain *c, zVec3D *p, zVec3D *am)
{
  register int i;
  zVec3D tp, tmp;

  zVec3DZero( am );
  for( i=0; i<rkChainLinkNum(c); i++ ){
    zXform3DInv( rkChainLinkWldFrame(c,i), p, &tp );
    rkLinkAM( rkChainLink(c,i), &tp, &tmp );
    zMulMat3DVec3DDRC( rkChainLinkWldAtt(c,i), &tmp );
    zVec3DAddDRC( am, &tmp );
  }
  return am;
}

/* kinetic energy of a kinematic chain. */
double rkChainKE(rkChain *c)
{
  register int i;
  double energy = 0;

  for( i=0; i<rkChainLinkNum(c); i++ )
    energy += rkLinkKE( rkChainLink(c,i) );
  return energy;
}

/* inertia matrix and bias force vector of a kinematic chain by the unit vector method. */
bool rkChainInertiaMatBiasVec(rkChain *chain, zMat inertia, zVec bias)
{
  register int i, j, k;
  zVecStruct h;
  double acc[] = { 0, 0, 0, 0, 0, 0 };

  if( !zMatIsSqr( inertia ) || !zMatColVecSizeIsEqual( inertia, bias ) ||
      zVecSizeNC( bias ) != rkChainJointSize(chain) ){
    ZRUNERROR( RK_ERR_MAT_VEC_SIZMISMATCH );
    return false;
  }
  /* bias force vector */
  rkChainSetJointAccAll( chain, NULL );
  rkChainUpdateID( chain );
  rkChainGetJointTrqAll( chain, bias );
  /* inertia matrix */
  h.size = zVecSizeNC( bias );
  for( i=j=0; j<rkChainLinkNum(chain); j++ )
    for( k=0; k<rkChainLinkJointSize(chain,j); k++, i++ ){
      if( i >= zVecSizeNC(bias) ){
        ZRUNERROR( RK_ERR_FATAL );
        return false;
      }
      h.buf = zMatRowBuf( inertia, i );
      acc[k] = 1;
      rkChainLinkJointSetAcc( chain, j, acc );
      rkChainUpdateID( chain );
      rkChainGetJointTrqAll( chain, &h );
      zVecSubDRC( &h, bias );
      acc[k] = 0;
      rkChainLinkJointSetAcc( chain, j, acc );
    }
  return true;
}

/* net external wrench applied to a kinematic chain. */
zVec6D *rkChainNetExtWrench(rkChain *c, zVec6D *w)
{
  register int i;
  zVec6D ew;

  zVec6DZero( w );
  for( i=0; i<rkChainLinkNum(c); i++ ){
    rkLinkNetExtWrench( rkChainLink(c,i), &ew );
    if( zVec6DEqual( &ew, ZVEC6DZERO ) ) continue;
    zMulMat3DVec6DDRC( rkChainLinkWldAtt(c,i), &ew );
    zVec6DAngShiftDRC( &ew, rkChainLinkWldPos(c,i) );
    zVec6DAddDRC( w, &ew );
  }
  return w;
}

/* destroy external wrench list attached to a kinematic chain. */
void rkChainExtWrenchDestroy(rkChain *c)
{
  register int i;

  for( i=0; i<rkChainLinkNum(c); i++ )
    rkLinkExtWrenchDestroy( rkChainLink(c,i) );
}

/* set offset values of links of a kinematic chain. */
void rkChainSetOffset(rkChain *c)
{
  register int i, s;

  for( i=0, s=0; i<rkChainLinkNum(c); i++ )
    if( rkChainLinkJointSize(c,i) > 0 ){
      rkLinkSetOffset( rkChainLink(c,i), s );
      s += rkChainLinkJointSize(c,i);
    } else
      rkLinkSetOffset( rkChainLink(c,i), -1 );
}

/* make a list of vertices of a kinematic chain. */
zVec3DList *rkChainVertList(rkChain *chain, zVec3DList *vl)
{
  rkLink *l;
  zShapeListCell *sc;
  zShape3D s;
  zVec3D v;
  register int i, j;

  zListInit( vl );
  for( i=0; i<rkChainLinkNum(chain); i++ ){
    l = rkChainLink(chain,i);
    zListForEach( rkLinkShapeList(l), sc ){
      if( sc->data->com == &zeo_shape3d_ph_com ){
        for( j=0; j<zShape3DVertNum(sc->data); j++ ){
          zXform3D( rkLinkWldFrame(l), zShape3DVert(sc->data,j), &v );
          if( !zVec3DListAdd( vl, &v ) ) return NULL;
        }
      } else{
        zShape3DClone( sc->data, &s, NULL );
        zShape3DXform( sc->data, rkLinkWldFrame(l), &s );
        if( !zShape3DToPH( &s ) ) return NULL;
        if( !zVec3DListAppendArray( vl, &zShape3DPH(&s)->vert ) ) vl = NULL;
        zShape3DDestroy( &s );
        if( !vl ) return NULL;
      }
    }
  }
  return vl;
}

/* generate the bounding ball of a kinematic chain. */
zSphere3D *rkChainBBall(rkChain *chain, zSphere3D *bb)
{
  zVec3DList pl;

  if( rkChainVertList( chain, &pl ) )
    zBBall3DPL( bb, &pl, NULL );
  else
    bb = NULL;
  zVec3DListDestroy( &pl );
  return bb;
}

/* ZTK */

static void *_rkChainNameFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  return zNameSet( (rkChain*)obj, ZTKVal(ztk) ) ? obj : NULL;
}

static void _rkChainNameFPrintZTK(FILE *fp, int i, void *obj){
  fprintf( fp, "%s\n", zName((rkChain*)obj) );
}

static ZTKPrp __ztk_prp_rkchain_chain[] = {
  { "name", 1, _rkChainNameFromZTK, _rkChainNameFPrintZTK },
};

static void *_rkChainChainFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  return ZTKEvalKey( obj, arg, ztk, __ztk_prp_rkchain_chain );
}
static void *_rkChainMotorFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  return rkMotorFromZTK( zArrayElemNC(rkChainMotor((rkChain*)obj),i), ztk );
}
static void *_rkChainLinkFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  return rkLinkFromZTK( rkChainLink((rkChain*)obj,i),
    &((rkChain*)obj)->link,
    rkChainShape((rkChain*)obj) ? &rkChainShape((rkChain*)obj)->shape : NULL,
    rkChainMotor((rkChain*)obj), ztk ) ? obj : NULL;
}
static void *_rkChainLinkConnectFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  return rkLinkConnectFromZTK( rkChainLink((rkChain*)obj,i),
    &((rkChain*)obj)->link, ztk ) ? obj : NULL;
}

static void _rkChainChainFPrintZTK(FILE *fp, int i, void *obj){
  ZTKPrpKeyFPrint( fp, obj, __ztk_prp_rkchain_chain );
}

static void *_rkChainInitPosFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  zVec3DFromZTK( rkChainOrgPos((rkChain*)obj), ztk );
  return obj;
}
static void *_rkChainInitAttFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  zMat3DFromZTK( rkChainOrgAtt((rkChain*)obj), ztk );
  return obj;
}
static void *_rkChainInitFrameFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  zFrame3DFromZTK( rkChainOrgFrame((rkChain*)obj), ztk );
  return obj;
}
static void *_rkChainInitJointFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  rkLink *link;
  zArrayFindName( &((rkChain*)obj)->link, ZTKVal(ztk), link );
  if( !link ){
    ZRUNERROR( RK_ERR_LINK_UNKNOWN, ZTKVal(ztk) );
    return NULL;
  }
  ZTKValNext( ztk );
  rkLinkJoint(link)->com->_dis_fromZTK( rkLinkJoint(link)->prp, 0, NULL, ztk );
  return obj;
}

static void _rkChainInitPosFPrintZTK(FILE *fp, int i, void *obj){
  zVec3DFPrint( fp, rkChainOrgPos((rkChain*)obj) );
}
static void _rkChainInitAttFPrintZTK(FILE *fp, int i, void *obj){
  zMat3DFPrint( fp, rkChainOrgAtt((rkChain*)obj) );
}

static ZTKPrp __ztk_prp_rkchain_initkey[] = {
  { "pos", 1, _rkChainInitPosFromZTK, _rkChainInitPosFPrintZTK },
  { "att", 1, _rkChainInitAttFromZTK, _rkChainInitAttFPrintZTK },
  { "frame", 1, _rkChainInitFrameFromZTK, NULL },
  { "joint", -1, _rkChainInitJointFromZTK, NULL },
};

static void *_rkChainInitFromZTK(void *obj, int i, void *arg, ZTK *ztk)
{
  if( !ZTKEvalKey( obj, NULL, ztk, __ztk_prp_rkchain_initkey ) ) return NULL;
  return obj;
}

static void _rkChainInitFPrintZTK(FILE *fp, int i, void *obj)
{
  register int k;
  rkLink *link;

  ZTKPrpKeyFPrint( fp, obj, __ztk_prp_rkchain_initkey );
  for( k=0; k<rkChainLinkNum((rkChain*)obj); k++ ){
    link = rkChainLink((rkChain*)obj,k);
    if( rkLinkJointSize(link) == 0 || rkJointIsNeutral( rkLinkJoint(link) ) ) continue;
    fprintf( fp, "joint: %s ", zName(link) );
    rkLinkJoint(link)->com->_dis_fprintZTK( fp, 0, rkLinkJoint(link)->prp );
  }
}

static ZTKPrp __ztk_prp_tag_rkchain_optic[] = {
  { ZTK_TAG_OPTIC, -1, NULL, NULL },
};

static ZTKPrp __ztk_prp_tag_rkchain_shape[] = {
  { ZTK_TAG_SHAPE, -1, NULL, NULL },
};

static ZTKPrp __ztk_prp_tag_rkchain_motor[] = {
  { ZTK_TAG_RKMOTOR, -1, _rkChainMotorFromZTK, NULL },
};

static ZTKPrp __ztk_prp_tag_rkchain_link[] = {
  { ZTK_TAG_RKLINK, -1, _rkChainLinkFromZTK, NULL },
};

static ZTKPrp __ztk_prp_tag_rkchain_connection[] = {
  { ZTK_TAG_RKLINK, -1, _rkChainLinkConnectFromZTK, NULL },
};

static ZTKPrp __ztk_prp_tag_rkchain[] = {
  { ZTK_TAG_RKCHAIN, 1, _rkChainChainFromZTK, _rkChainChainFPrintZTK },
  { ZTK_TAG_INIT, 1, _rkChainInitFromZTK, NULL },
};

rkChain *rkChainFromZTK(rkChain *chain, ZTK *ztk)
{
  int num_motor, num_link;

  if( ( num_motor = ZTKCountTag( ztk, ZTK_TAG_RKMOTOR ) ) > 0 ){
    if( !( rkChainMotor(chain) = zAlloc( rkMotorArray, 1 ) ) ){
      ZALLOCERROR();
      return NULL;
    }
    zArrayAlloc( rkChainMotor(chain), rkMotor, num_motor );
    if( zArraySize(rkChainMotor(chain)) != num_motor ) return NULL;
  }
  num_link = ZTKCountTag( ztk, ZTK_TAG_RKLINK );
  zArrayAlloc( &chain->link, rkLink, num_link );
  if( rkChainLinkNum(chain) != num_link ) return NULL;
  if( rkChainLinkNum(chain) == 0 ){
    ZRUNWARN( RK_WARN_CHAIN_EMPTY );
    return NULL;
  }
  ZTKEvalTag( chain, NULL, ztk, __ztk_prp_tag_rkchain_optic );
  ZTKEvalTag( chain, NULL, ztk, __ztk_prp_tag_rkchain_shape );
  ZTKEvalTag( chain, NULL, ztk, __ztk_prp_tag_rkchain_motor );
  ZTKEvalTag( chain, NULL, ztk, __ztk_prp_tag_rkchain_link );
  ZTKEvalTag( chain, NULL, ztk, __ztk_prp_tag_rkchain_connection );
  ZTKEvalTag( chain, NULL, ztk, __ztk_prp_tag_rkchain );
  if( rkChainCalcMass(chain) == 0 )
    rkChainSetMass( chain, 1.0 ); /* dummy weight */
  rkChainSetOffset( chain ); /* offset value arrangement */
  rkChainUpdateFK( chain );
  rkChainUpdateID( chain );
  return chain;
}

/* print information of a kinematic chain out to the current position of a file. */
void rkChainFPrintZTK(FILE *fp, rkChain *chain)
{
  register int i;

  ZTKPrpTagFPrint( fp, chain, __ztk_prp_tag_rkchain );
  fprintf( fp, "\n" );
  if( rkChainShape(chain) )
    zMShape3DFPrintZTK( fp, rkChainShape(chain) );
  if( rkChainMotor(chain) )
    rkMotorArrayFPrintZTK( fp, rkChainMotor(chain) );
  for( i=0; i<rkChainLinkNum(chain); i++ ){
    fprintf( fp, "[%s]\n", ZTK_TAG_RKLINK );
    rkLinkFPrintZTK( fp, rkChainLink(chain,i) );
  }
  fprintf( fp, "[%s]\n", ZTK_TAG_INIT );
  _rkChainInitFPrintZTK( fp, 0, chain );
}

/* read a ZTK file and create a new kinematic chain. */
rkChain *rkChainReadZTK(rkChain *chain, char filename[])
{
  ZTK ztk;

  ZTKInit( &ztk );
  if( ZTKParse( &ztk, filename ) ){
    /* read optical infos and shapes */
    rkChainInit( chain );
    if( ZTKCountTag( &ztk, ZTK_TAG_SHAPE ) > 0 ){
      if( !( rkChainShape(chain) = zAlloc( zMShape3D, 1 ) ) ){
        ZALLOCERROR();
        return NULL;
      }
      if( !zMShape3DFromZTK( rkChainShape(chain), &ztk ) ) return NULL;
    }
    /* read robot name, motors and links */
    chain = rkChainFromZTK( chain, &ztk );
  } else
    chain = NULL;
  ZTKDestroy( &ztk );
  return chain;
}

/* write information of a kinematic chain to a file in ZTK format. */
bool rkChainWriteZTK(rkChain *c, char filename[])
{
  FILE *fp;

  if( !( fp = zOpenZTKFile( filename, "w" ) ) ) return false;
  rkChainFPrintZTK( fp, c );
  fclose(fp);
  return true;
}

static ZTKPrp __ztk_prp_tag_rkchain_init[] = {
  { ZTK_TAG_INIT, 1, _rkChainInitFromZTK, _rkChainInitFPrintZTK },
};

rkChain *rkChainInitFromZTK(rkChain *chain, ZTK *ztk)
{
  ZTKEvalTag( chain, NULL, ztk, __ztk_prp_tag_rkchain_init );
  rkChainUpdateFK( chain );
  rkChainUpdateID( chain );
  return chain;
}

void rkChainInitFPrintZTK(FILE *fp, rkChain *chain)
{
  ZTKPrpTagFPrint( fp, chain, __ztk_prp_tag_rkchain_init );
  fprintf( fp, "\n" );
}

rkChain *rkChainInitReadZTK(rkChain *chain, char filename[])
{
  ZTK ztk;

  ZTKInit( &ztk );
  ZTKParse( &ztk, filename );
  chain = rkChainInitFromZTK( chain, &ztk );
  ZTKDestroy( &ztk );
  return chain;
}

bool rkChainInitWriteZTK(rkChain *chain, char filename[])
{
  FILE *fp;

  if( !( fp = zOpenZTKFile( filename, "w" ) ) ) return false;
  rkChainInitFPrintZTK( fp, chain );
  fclose( fp );
  return true;
}

/* print current posture of a kinematic chain out to a file. */
void rkChainPostureFPrint(FILE *fp, rkChain *c)
{
  register int i;

  fprintf( fp, "Chain : %s\n", zName(c) );
  for( i=0; i<rkChainLinkNum(c); i++ )
    rkLinkPostureFPrint( fp, rkChainLink(c,i) );
}

/* print connection of a kinematic chain out to a file. */
void rkChainConnectionFPrint(FILE *fp, rkChain *c)
{
  fprintf( fp, "Chain : %s\n", zName(c) );
  rkLinkConnectionFPrint( fp, rkChainRoot(c), 0 );
}

/* print external wrench exerted to a kinematic chain out to a file. */
void rkChainExtWrenchFPrint(FILE *fp, rkChain *c)
{
  register int i;

  for( i=0; i<rkChainLinkNum(c); i++ )
    if( zListSize(rkChainLinkExtWrench(c,i)) != 0 ){
      fprintf( fp, "[%s]\n", rkChainLinkName(c,i) );
      rkLinkExtWrenchFPrint( fp, rkChainLink(c,i) );
    }
}
