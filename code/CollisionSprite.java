package library;

public class CollisionSprite extends Sprite
{
    // Collision check class
    CollisionCheck colCheck = new CollisionCheck();

    // Translated mesh data
    Face[] transFace;
    Point offset = new Point();

    public static int EC_FLOOR = 0, EC_WALL = 1, EC_MAX = 2;

    static float NOSE = 0.05f;
    static float BNOSE = 0.05f;
    static float WALL_GLIDE_SPEED = 0.0f;

    public int compareTo(Object3D obj)
    {
        return 0;
    }

    /************************************************************************
    *    desc:  Init the sprite with the path to the mesh file
    *
    *    param: std::string meshPath - path to mesh xml
    *
    *    return: bool - true on success or false on fail
    ************************************************************************/
    public boolean init( String meshPath )
    {
        mesh = MeshManager.getInstance().getCollisionMesh( meshPath );

        if( mesh == null)
        {
            return false;
        }

        // Allocate our transformation face array
        transFace = new Face[mesh.getFaceCount()];

        for(int i = 0; i < transFace.length; i++)
        {
            transFace[i] = new Face();
        }

        return true;

    }   // Init


    /************************************************************************
    *    desc:  Transform the sprites data. This is for updating
    *			objects to collide with other objects
    *
    *    param: Matrix matrix - passed in matrix class
    ************************************************************************/
    public void transform()
    {
        // Do our translations and rotations in a temporary matrix
        Matrix noScaleMatrix = new Matrix();
        Matrix scaleMatrix = new Matrix();

        // Handle the scaling
        scaleMatrix.scale( scale );

        // Set the matrix to world view
        setMatrixToWorldView( noScaleMatrix );
        setMatrixToWorldView( scaleMatrix );

        // Transform the center point
        transform( scaleMatrix );

        // Transform the mesh
        // NOTE: Since we are tranforming our collision mesh, you can't
        // scale normals. So for this, we have a seperate matix without scale.
        mesh.transform( transFace, scale, scaleMatrix, noScaleMatrix );

    }	// Transform

    /************************************************************************
    *    desc:  Transform the sprites data. This is for updating
    *			objects to collide with other objects
    *
    *    param: Matrix matrix - passed in matrix class
    ************************************************************************/
    public void transformB(Matrix matrix)
    {
        // Do our translations and rotations in a temporary matrix
        Matrix noScaleMatrix = new Matrix();
        Matrix scaleMatrix = new Matrix();

        // Handle the scaling
        scaleMatrix.scale( scale );

        // Set the matrix to world view
        setMatrixToWorldView( noScaleMatrix );
        setMatrixToWorldView( scaleMatrix );

        // Merge in the passed in matrix to convert to world view
        scaleMatrix.mergeMatrix( matrix );
        noScaleMatrix.mergeMatrix( matrix );

        // Transform the center point
        transform( scaleMatrix );

        // Transform the mesh
        // NOTE: Since we are tranforming our collision mesh, you can't
        // scale normals. So for this, we have a seperate matix without scale.
        mesh.transform( transFace, scale, scaleMatrix, noScaleMatrix );

    }	// Transform


    /************************************************************************
    *    desc:  Check for collision and react to it
    *
    *    param: CMatric & matrix - passed in matrix class
    *			CPoint center - Center point of the object
    *           float radius - radius of the object
    *           float floorPadding - padding for collision
    *           float wallPadding - padding for collision
    *
    *    ret: CCollisionCheck & - All collision information
    ************************************************************************/
    public CollisionCheck reactToCollision( Matrix matrix, Point center, float radius, float floorPadding, float wallPadding )
    {
        // reset the collision class
        colCheck.reset();

        // Do our translations and rotations in a temporary matrix
        Matrix noScaleMatrix = new Matrix();
        Matrix scaleMatrix = new Matrix();

        // Handle the scaling
        scaleMatrix.scale( scale );

        // Set the matrix to world view
        setMatrixToWorldView( noScaleMatrix );
        setMatrixToWorldView( scaleMatrix );

        // Merge in the passed in matrix to convert to world view
        scaleMatrix.mergeMatrix( matrix );
        noScaleMatrix.mergeMatrix( matrix );

        // Transform the center point
        transform(scaleMatrix);

        // See if we are colliding with the object before we
        // transform the data and do the real tests
        if( isCollision_BoundingSphereMesh( center, radius ) )
        {
            // Inc our stat counter to keep track of what is going on.
            StatCounter.getInstance().incCollisionCounter();

            // Transform the mesh
            // NOTE: Since we are tranforming our collision mesh, you can't
            // scale normals. So for this, we have a seperate matix without scale.
            mesh.transform( transFace, scale, scaleMatrix, noScaleMatrix );

            // Do the bounding sphere check on each of the poly faces
            if( isCollision_BoundingSphere( center, radius ) )
            {
                // Do the intersection check
                isCollision_Intersection( center, floorPadding, wallPadding );
            }
        }

        return colCheck;
    }	// ReactToCollision


    /************************************************************************
    *    desc:  Check for collision and react to it
    *
    *    param: CPoint center - Center point of the object
    *           float radius - radius of the object
    *           float floorPadding - padding for collision
    *           float wallPadding - padding for collision
    *
    *    ret: CCollisionCheck & - All collision information
    ************************************************************************/
    public CollisionCheck reactToCollision( Point center, float radius, float floorPadding, float wallPadding )
    {
        // reset the collision class
        colCheck.reset();

        // See if we are colliding with the object before we
        // transform the data and do the real tests
        if( isCollision_BoundingSphereMesh( center, radius ) )
        {
            // Do the bounding sphere check on each of the poly faces
            if( isCollision_BoundingSphere( center, radius ) )
            {
                // Do the intersection check
                isCollision_Intersection( center, floorPadding, wallPadding );
            }
        }

            return colCheck;
    }	// ReactToCollision


    /************************************************************************
    *    desc:  Do bounding sphere mesh collision check on the whole mesh.
    *
    *    param: CPoint center - Center point of the object
    *           float radius - radius of the object
    ************************************************************************/
    public boolean isCollision_BoundingSphereMesh( Point center, float radius )
    {
        boolean result = false;

        // Get the length from the object center to the passed in center
        float distance = trans_center.getLength( center );

        // If the distance is less then the two radiuses, we might be colliding
        if( distance <= getRadius() + radius )
        {
            result = true;
        }

        return result;

    }	// isCollision_BoundingSphereMesh

    /************************************************************************
    *    desc:  Do bounding sphere face collision check. This is to find the
    *           closest faces to do the intersection check against.
    *
    *           NOTE: If you're bouncing on the floor or when hitting the wall
    *                 your radius is too small. You will bounce some when going
    *                 into the corner.
    *
    *    param: CPoint center - Center point of the object
    *           float radius - radius of the object
    ************************************************************************/
    boolean isCollision_BoundingSphere( Point center, float radius )
    {
        //boolean checkFloor = true;
        boolean result = false;

        // Go through the entire mesh and find the closest collision
        for( int i = 0; i < mesh.getFaceCount(); i++ )
        {
            // Get the length from the face center to the passed in center
            float distance = transFace[i].center.getLength( center );

            //System.out.println( distance + " | " + transFace[i].radius + " | " + radius );

            // If the distance is less then the two radiuses, we might be colliding
            if( distance < transFace[i].radius + radius )
            {
                // Check the y normal to see if it is a floor
                if( transFace[i].normal.y > 0.4f )
                {
                    if( distance < colCheck.col[EC_FLOOR].distance )
                    //if( checkFloor )
                    {
                        // Make sure the plane is not back facing us
                        if( transFace[i].isFacingPlane( center ) )
                        {
                            //if(transFace[i].isPointInTri(center))
                            {
                                colCheck.col[EC_FLOOR].distance = distance;
                                colCheck.col[EC_FLOOR].index = i;
                                result = true;
                                //checkFloor = false;
                            }
                        }
                    }
                }
                // if it's not a floor, then it's a wall
                else
                {
                    if( distance < colCheck.col[EC_WALL].distance )
                    {
                        // Make sure the plane is not back facing us
                        if( transFace[i].isFacingPlane( center ) )
                        {
                            colCheck.col[EC_WALL].distance = distance;
                            colCheck.col[EC_WALL].index = i;
                            result = true;
                        }
                    }
                }
            }
        }

            return result;
    }	// isCollision_BoundingSphere


    /************************************************************************
    *    desc:  Last check for collision. Much more accurate then bounding
    *           spheres and also much slower which is why this check is
    *           is performed after a bounding sphere's tests are finished.
    *           Also, intersection collision is testing if the plane
    *           was pierced by a point. We are talking a plain, not a
    *           polygon. So the test is not confined to the size of the
    *           polygon, but the plain it is on. A plain is infanit.
    *
    *    param: CollisionCheck colCheck - class for storing collision info
    *		Point center - point to check.
    *           float floorPadding - padding for collision
    *           float wallPadding - padding for collision
    *
    *           *NOTE: If in First Persion point of view, use 0,0,0 as your
    *                  x,y,z point.
    ************************************************************************/
    public boolean isCollision_Intersection( Point center, float floorPadding, float wallPadding )
    {
        // This is only used for the walls and is to give a little extra push away
        // from the plane so that the next intersection check fails. This keeps you from
        // getting stuck to the walls.
            
        // Do intersection check if we had a wall collision
        for( int i = 0; i < EC_MAX; i++ )
        {
            if( colCheck.col[i].isCollision() )
            {
                float padding;

                if( i == EC_FLOOR )
                {
                    padding = floorPadding;
                }
                else
                {
                    padding = wallPadding;
                }

                // get the closest face we collided with during the bounding sphere check
                Face faceTmp = transFace[colCheck.col[i].index];

                // Did we pierce the plane?
                colCheck.col[i].planePierceOffset = faceTmp.angleToPlane( center ) + -padding;

                // Did we pierce the plane?
                if( colCheck.col[i].planePierceOffset < 0.0f )
                {                    
                    // Set the flag to indicate we had a collision
                    colCheck.setCollision( true );

                    // Get the elapsed time for wall gliding
                    float elapsedTime = ElapsedTime.getInstance().getElapsedTime();

                    // Set the offset based on what we collided against
                    if( i == EC_FLOOR )
                    {
                        // Compute the y delta
                        if( faceTmp.normal.y != 0.0f )
                        {
                            offset.y = (colCheck.col[i].planePierceOffset / faceTmp.normal.y);
                        }
                        else
                        {
                            offset.y = colCheck.col[i].planePierceOffset;
                        }
                    }
                    else
                    {
                        // Glide along the wall directly in front or behind us
                        if( Math.abs(faceTmp.normal.z) > Math.abs(faceTmp.normal.x) )
                        {
                            // The wall is in front of us
                            if( faceTmp.normal.z < 0.0f )
                            {
                                offset.z = (-colCheck.col[i].planePierceOffset / -faceTmp.normal.z) + NOSE;

                                // x offset for wall gliding
                                offset.x = (faceTmp.normal.x * (-WALL_GLIDE_SPEED * elapsedTime));
                            }
                            // The wall is behind us
                            else
                            {
                                offset.z = (colCheck.col[i].planePierceOffset / faceTmp.normal.z) - BNOSE;

                                // x offset for wall gliding
                                offset.x = faceTmp.normal.x * (-WALL_GLIDE_SPEED * elapsedTime);
                            }
                        }
                        // Glide along the wall to our right or left side
                        else
                        {
                            // The wall is on our right side
                            if( faceTmp.normal.x < 0.0f )
                            {
                                offset.x = (colCheck.col[i].planePierceOffset / faceTmp.normal.x ) + BNOSE;

                                // z offset for wall gliding
                                offset.z = faceTmp.normal.z * (-WALL_GLIDE_SPEED * elapsedTime);
                            }
                            // The wall is on our left side
                            else
                            {
                                offset.x = (-colCheck.col[i].planePierceOffset / -faceTmp.normal.x ) - NOSE;

                                // z offset for wall gliding
                                offset.z = (faceTmp.normal.z * (-WALL_GLIDE_SPEED * elapsedTime));
                            }

                        }
                    }
                    
                    // Record this information in case we need it later
                    colCheck.col[i].offset.equ(offset);
                    colCheck.col[i].normal.equ(faceTmp.normal);
                    colCheck.col[i].center.equ(faceTmp.center);
                }
            }
        }

        return colCheck.isCollision();
    }   // IsCollision_Intersection

}
