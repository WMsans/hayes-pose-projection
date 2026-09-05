using Godot;

public enum BoneSide
{
	Right,
	Left,
	Spine
}

public class Bone
{
	public int A;
	public int B;
	public BoneSide Side;

	public Bone(int a, int b, BoneSide side)
	{
		A = a;
		B = b;
		Side = side;
	}
}

// The 14 joints in joint-names.txt have no explicit connectivity, so the
// skeleton is spelled out here. Colours match the sample figure on the
// challenge page: right limbs red, left limbs blue, spine black.
public static class Bones
{
	public static readonly Bone[] All = new Bone[]
	{
		new Bone(0, 1, BoneSide.Right),   // Hip - RHip
		new Bone(1, 2, BoneSide.Right),   // RHip - RKnee
		new Bone(2, 3, BoneSide.Right),   // RKnee - RAnkle
		new Bone(0, 4, BoneSide.Left),    // Hip - LHip
		new Bone(4, 5, BoneSide.Left),    // LHip - LKnee
		new Bone(5, 6, BoneSide.Left),    // LKnee - LAnkle
		new Bone(0, 7, BoneSide.Spine),   // Hip - Neck
		new Bone(7, 8, BoneSide.Left),    // Neck - LUpperArm
		new Bone(8, 9, BoneSide.Left),    // LUpperArm - LElbow
		new Bone(9, 10, BoneSide.Left),   // LElbow - LWrist
		new Bone(7, 11, BoneSide.Right),  // Neck - RUpperArm
		new Bone(11, 12, BoneSide.Right), // RUpperArm - RElbow
		new Bone(12, 13, BoneSide.Right)  // RElbow - RWrist
	};

	public static Color ColorFor(BoneSide side)
	{
		if (side == BoneSide.Right)
		{
			return new Color(0.85f, 0.15f, 0.15f);
		}
		if (side == BoneSide.Left)
		{
			return new Color(0.15f, 0.30f, 0.85f);
		}
		return new Color(0.10f, 0.10f, 0.10f);
	}
}
