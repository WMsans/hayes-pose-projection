using Godot;
using Xunit;

public class BonesTests
{
	[Fact]
	public void ThereAreThirteenBones()
	{
		Assert.Equal(13, Bones.All.Length);
	}

	[Fact]
	public void EveryEndpointIsAValidJointIndex()
	{
		for (int i = 0; i < Bones.All.Length; i++)
		{
			Assert.InRange(Bones.All[i].A, 0, 13);
			Assert.InRange(Bones.All[i].B, 0, 13);
			Assert.NotEqual(Bones.All[i].A, Bones.All[i].B);
		}
	}

	[Fact]
	public void EveryJointIsConnectedToSomething()
	{
		bool[] seen = new bool[14];
		for (int i = 0; i < Bones.All.Length; i++)
		{
			seen[Bones.All[i].A] = true;
			seen[Bones.All[i].B] = true;
		}
		for (int j = 0; j < seen.Length; j++)
		{
			Assert.True(seen[j], "joint " + j + " has no bone");
		}
	}

	[Fact]
	public void LimbChainsAreWiredCorrectly()
	{
		Assert.True(HasBone(0, 1) && HasBone(1, 2) && HasBone(2, 3));    // right leg
		Assert.True(HasBone(0, 4) && HasBone(4, 5) && HasBone(5, 6));    // left leg
		Assert.True(HasBone(0, 7));                                      // spine
		Assert.True(HasBone(7, 8) && HasBone(8, 9) && HasBone(9, 10));   // left arm
		Assert.True(HasBone(7, 11) && HasBone(11, 12) && HasBone(12, 13)); // right arm
	}

	[Fact]
	public void SidesAreAssignedToMatchTheSampleFigure()
	{
		Assert.Equal(BoneSide.Right, SideOf(2, 3));
		Assert.Equal(BoneSide.Left, SideOf(5, 6));
		Assert.Equal(BoneSide.Spine, SideOf(0, 7));
		Assert.Equal(BoneSide.Left, SideOf(9, 10));
		Assert.Equal(BoneSide.Right, SideOf(12, 13));
	}

	[Fact]
	public void ColoursAreRedBlueBlack()
	{
		Assert.Equal(new Color(0.85f, 0.15f, 0.15f), Bones.ColorFor(BoneSide.Right));
		Assert.Equal(new Color(0.15f, 0.30f, 0.85f), Bones.ColorFor(BoneSide.Left));
		Assert.Equal(new Color(0.10f, 0.10f, 0.10f), Bones.ColorFor(BoneSide.Spine));
	}

	private static bool HasBone(int a, int b)
	{
		for (int i = 0; i < Bones.All.Length; i++)
		{
			if (Bones.All[i].A == a && Bones.All[i].B == b)
			{
				return true;
			}
		}
		return false;
	}

	private static BoneSide SideOf(int a, int b)
	{
		for (int i = 0; i < Bones.All.Length; i++)
		{
			if (Bones.All[i].A == a && Bones.All[i].B == b)
			{
				return Bones.All[i].Side;
			}
		}
		throw new Xunit.Sdk.XunitException("no bone " + a + "-" + b);
	}
}
