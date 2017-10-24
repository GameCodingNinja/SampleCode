package library;

import library.*;
import javax.media.opengl.*;

public class GLSprite extends Sprite 
{
    private boolean culled = false;

    /************************************************************************
    *    desc:  Init the sprite with the path to the mesh file
    *
    *    param: String meshPath - path to mesh xml
    ************************************************************************/
    public void init( String meshPath )
    {
        mesh = MeshManager.getInstance().getGLMesh( meshPath );
    }

    /************************************************************************
    *    desc:  Test position of object in frustum to see if it can
    *           be culled. This assumes a 45 degree view area.
    *
    *    ret:   bool - True indicates object is outside frustum
    ************************************************************************/
    public boolean cullMesh_BoundSphere()
    {
        //////////////////////////////////////////
        // Test against the far & near plains
        //////////////////////////////////////////
        // if this sprite is big enough that we are inside it, no sence checking it
        // because we are currently inside the radius and should always render it.
        if( Math.abs(trans_center.z) > getRadiusSqrt() )
        {
            // Test against the far z plane
            if( -trans_center.z - getRadiusSqrt() > GLWindow.getInstance().getMaxZDist() )
            {
                return true;
            }

            // test against the near z plane
            if( -trans_center.z + getRadiusSqrt() < GLWindow.getInstance().getMinZDist() )
            {
                return true;
            }

            //////////////////////////////////////////
            // Test against the Y side of the frustum
            //////////////////////////////////////////

            // Find the farthest Z point
            float farPoint = trans_center.z * GLWindow.getInstance().getFrustrumYRatio();

            // test against the top of the y frustrum
            if( trans_center.y - getRadiusSqrt() > -farPoint )
            {
                return true;
            }

            // test against the bottom of the y frustrum
            if( trans_center.y + getRadiusSqrt() < farPoint )
            {
                return true;
            }
   
            //////////////////////////////////////////
            // Test against the X side of the frustum
            /////////////////////////////////////////

            farPoint = trans_center.z * GLWindow.getInstance().getSquarePercentage();
            
            // test against the left of the x frustrum
            if( trans_center.x + getRadiusSqrt() < farPoint )
            {
                return true;
            }

            // test against the right of the x frustrum
            if( trans_center.x - getRadiusSqrt() > -farPoint )
            {
                 return true;
            }
        }

        // if we made it this far, the object was not culled
        return false;
    }   // CullMesh_BoundSphere


    /************************************************************************
    *    desc:  Render this mesh to the back buffer
    *
    *    param: GL gl - OpenGL object
    ************************************************************************/
    public void render( GL gl )
    {
        if( !culled && visible )
        {
            StatCounter.getInstance().incDisplayCounter();

            // push a matrix onto the stack
            gl.glPushMatrix();

            // Transform sprite to world view
            setMatrixToWorldView( gl );

            // Render the mesh
            mesh.render( gl );

            // pop a matrix off the stack
            gl.glPopMatrix();
        }
    }


    /************************************************************************
    *    desc:  Render this mesh to the back buffer
    *
    *    param: GL gl - OpenGL object
    *           Matrix matrix - passed in matrix
    ************************************************************************/
    public void render( GL gl, Matrix matrix )
    {
        // Do our translations and rotations in a temporary matrix
	Matrix tmpMatrix = new Matrix();

	// Add in the scaling
	tmpMatrix.scale( scale );

	// Set to the temporary matrix to world view
	setMatrixToWorldView( tmpMatrix );

	// Merge in the passed in matrix to convert to camera view
	tmpMatrix.mergeMatrix( matrix );

	// Transform the center point - needed for CullMesh_BoundSphere()
	transform( tmpMatrix );

        if( visible && !cullMesh_BoundSphere() )
        {
            StatCounter.getInstance().incDisplayCounter();

            // push a matrix onto the stack
            gl.glPushMatrix();

            // Transform sprite to world view
            setMatrixToWorldView( gl );

            // Render the mesh
            mesh.render( gl );

            // pop a matrix off the stack
            gl.glPopMatrix();
        }
    }
    /************************************************************************
    *    desc:  Render this mesh to the back buffer
    *
    *    param: GL gl - OpenGL object
    *           Matrix matrix - passed in matrix
    ************************************************************************/
    public void update( GL gl, Matrix matrix )
    {
        // Do our translations and rotations in a temporary matrix
	Matrix tmpMatrix = new Matrix();

	// Add in the scaling
	tmpMatrix.scale( scale );

	// Set to the temporary matrix to world view
	setMatrixToWorldView( tmpMatrix );

	// Merge in the passed in matrix to convert to camera view
	tmpMatrix.mergeMatrix( matrix );

	// Transform the center point - needed for CullMesh_BoundSphere()
	transform( tmpMatrix );

        culled = cullMesh_BoundSphere();
    }
}
