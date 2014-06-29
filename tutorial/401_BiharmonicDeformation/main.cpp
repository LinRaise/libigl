#include <igl/colon.h>
#include <igl/harmonic.h>
#include <igl/readOBJ.h>
#include <igl/readDMAT.h>
#include <igl/viewer/Viewer.h>
#include <algorithm>
#include <iostream>

double bc_frac = 1.0;
double bc_dir = -0.03;
bool deformation_field = false;
Eigen::MatrixXd V,U,V_bc,U_bc;
Eigen::VectorXd Z;
Eigen::MatrixXi F;
Eigen::VectorXi b;

bool pre_draw(igl::Viewer & viewer)
{
  using namespace Eigen;
  // Determine boundary conditions
  if(viewer.options.is_animating)
  {
    bc_frac += bc_dir;
    bc_dir *= (bc_frac>=1.0 || bc_frac<=0.0?-1.0:1.0);
  }

  const MatrixXd U_bc_anim = V_bc+bc_frac*(U_bc-V_bc);
  if(deformation_field)
  {
    MatrixXd D;
    MatrixXd D_bc = U_bc_anim - V_bc;
    igl::harmonic(V,F,b,D_bc,2,D);
    U = V+D;
  }else
  {
    igl::harmonic(V,F,b,U_bc_anim,2,U);
  }
  viewer.set_vertices(U);
  viewer.compute_normals();
  return false;
}

bool key_down(igl::Viewer &viewer, unsigned char key, int mods)
{
  switch(key)
  {
    case ' ':
      viewer.options.is_animating = !viewer.options.is_animating;
      break;
    case 'D':
    case 'd':
      deformation_field = !deformation_field;
      break;
  }
}

int main(int argc, char *argv[])
{
  using namespace Eigen;
  using namespace std;
  igl::readOBJ("../shared/decimated-max.obj",V,F);
  U=V;
  // S(i) = j: j<0 (vertex i not in handle), j >= 0 (vertex i in handle j)
  VectorXi S;
  igl::readDMAT("../shared/decimated-max-selection.dmat",S);
  igl::colon<int>(0,V.rows()-1,b);
  b.conservativeResize(stable_partition( b.data(), b.data()+b.size(), 
   [&S](int i)->bool{return S(i)>=0;})-b.data());

  // Boundary conditions directly on deformed positions
  U_bc.resize(b.size(),V.cols());
  V_bc.resize(b.size(),V.cols());
  for(int bi = 0;bi<b.size();bi++)
  {
    V_bc.row(bi) = V.row(b(bi));
    switch(S(b(bi)))
    {
      case 0:
        // Don't move handle 0
        U_bc.row(bi) = V.row(b(bi));
        break;
      case 1:
        // move handle 1 down
        U_bc.row(bi) = V.row(b(bi)) + RowVector3d(0,-50,0);
        break;
      case 2:
      default:
        // move other handles forward
        U_bc.row(bi) = V.row(b(bi)) + RowVector3d(0,0,-25);
        break;
    }
  }

  // Pseudo-color based on selection
  MatrixXd C(F.rows(),3);
  RowVector3d purple(80.0/255.0,64.0/255.0,255.0/255.0);
  RowVector3d gold(255.0/255.0,228.0/255.0,58.0/255.0);
  for(int f = 0;f<F.rows();f++)
  {
    if( S(F(f,0))>=0 && S(F(f,1))>=0 && S(F(f,2))>=0)
    {
      C.row(f) = purple;
    }else
    {
      C.row(f) = gold;
    }
  }

  // Plot the mesh with pseudocolors
  igl::Viewer viewer;
  viewer.set_mesh(U, F);
  viewer.options.show_lines = false;
  viewer.set_colors(C);
  viewer.options.trackball_angle << 0,sqrt(2.0),0,sqrt(2.0);
  viewer.options.trackball_angle.normalize();
  viewer.callback_pre_draw = &pre_draw;
  viewer.callback_key_down = &key_down;
  //viewer.options.is_animating = true;
  viewer.options.animation_max_fps = 30.;
  viewer.launch();
}