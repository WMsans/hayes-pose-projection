using Godot;

// The second implementation: let the engine's camera do the projection.
// It reads the live scene graph, so Main must have positioned the figure
// and aimed the camera before Project is called.
public class GodotProjector : IProjector
{
	private Camera3D _camera;
	private PoseFigure _figure;

	public GodotProjector(Camera3D camera, PoseFigure figure)
	{
		_camera = camera;
		_figure = figure;
	}

	public void Begin(PoseFrame frame, float focal)
	{
		// Nothing to precompute; the scene already holds the state.
	}

	public Vector2 Project(int jointIndex)
	{
		return _camera.UnprojectPosition(_figure.GetJointNode(jointIndex).GlobalPosition);
	}
}
