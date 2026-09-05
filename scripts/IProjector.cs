using Godot;

public enum ProjectionMethod
{
	ManualPinhole,
	GodotUnproject
}

// Two implementations exist on purpose, so each one checks the other.
public interface IProjector
{
	void Begin(PoseFrame frame, float focal);
	Vector2 Project(int jointIndex);
}
