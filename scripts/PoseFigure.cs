using Godot;

// The 3D skeleton: one Node3D per joint plus a mesh per bone. Named
// PoseFigure rather than Skeleton so it does not collide with Godot's
// own Skeleton3D.
public partial class PoseFigure : Node3D
{
	private const float JointRadius = 0.035f;
	private const float BoneRadius = 0.018f;

	private Node3D[] _joints;
	private MeshInstance3D[] _bones;

	public void Build()
	{
		_joints = new Node3D[PoseData.JointCount];
		for (int j = 0; j < _joints.Length; j++)
		{
			Node3D joint = new Node3D();
			joint.Name = "Joint" + j;
			AddChild(joint);
			_joints[j] = joint;

			SphereMesh sphere = new SphereMesh();
			sphere.Radius = JointRadius;
			sphere.Height = JointRadius * 2.0f;

			MeshInstance3D dot = new MeshInstance3D();
			dot.Mesh = sphere;
			dot.MaterialOverride = MakeMaterial(new Color(0.10f, 0.10f, 0.10f));
			joint.AddChild(dot);
		}

		_bones = new MeshInstance3D[Bones.All.Length];
		for (int b = 0; b < Bones.All.Length; b++)
		{
			CylinderMesh cylinder = new CylinderMesh();
			cylinder.TopRadius = BoneRadius;
			cylinder.BottomRadius = BoneRadius;
			cylinder.Height = 1.0f;

			MeshInstance3D bone = new MeshInstance3D();
			bone.Name = "Bone" + b;
			bone.Mesh = cylinder;
			bone.MaterialOverride = MakeMaterial(Bones.ColorFor(Bones.All[b].Side));
			AddChild(bone);
			_bones[b] = bone;
		}
	}

	public void ShowFrame(PoseFrame frame)
	{
		for (int j = 0; j < _joints.Length; j++)
		{
			_joints[j].Position = frame.JointsGodot[j];
		}

		for (int b = 0; b < _bones.Length; b++)
		{
			Vector3 a = frame.JointsGodot[Bones.All[b].A];
			Vector3 c = frame.JointsGodot[Bones.All[b].B];
			PlaceBone(_bones[b], a, c);
		}
	}

	public Node3D GetJointNode(int index)
	{
		return _joints[index];
	}

	// A CylinderMesh runs along its local Y axis, so the bone is moved to the
	// midpoint, turned to face the far end, then stretched to the joint
	// separation. Basis must be assigned before Scale: assigning a Basis
	// replaces any scale already on the node.
	private static void PlaceBone(MeshInstance3D bone, Vector3 a, Vector3 b)
	{
		Vector3 delta = b - a;
		float length = delta.Length();
		bone.Position = (a + b) * 0.5f;

		if (length > 0.0001f)
		{
			Vector3 dir = delta / length;
			// Any vector not parallel to dir will do as a reference for the
			// cross products; switch axes near the degenerate case.
			Vector3 reference = Vector3.Right;
			if (Mathf.Abs(dir.Dot(reference)) > 0.99f)
			{
				reference = Vector3.Forward;
			}
			Vector3 side = dir.Cross(reference).Normalized();
			Vector3 front = side.Cross(dir).Normalized();
			bone.Basis = new Basis(side, dir, front);
		}

		bone.Scale = new Vector3(1.0f, Mathf.Max(length, 0.0001f), 1.0f);
	}

	private static StandardMaterial3D MakeMaterial(Color color)
	{
		StandardMaterial3D material = new StandardMaterial3D();
		material.AlbedoColor = color;
		material.ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded;
		return material;
	}
}
