# Hayes Pose Projection Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a Godot 4.7 + C# interactive tool that projects the 20 given 3D skeletons to 2D, exports the coordinate table and images the Hayes challenge asks for, and implements the projection twice so each method verifies the other.

**Architecture:** All geometry and file-format logic lives in plain C# classes with no Godot node dependency, unit-tested with xUnit outside the engine. Engine-bound classes (scene, cameras, overlay, UI, export) are thin wrappers over that core, checked by an in-engine headless self-test. A Python reference implementation gives a third, independent opinion on every number.

**Tech Stack:** Godot 4.7.2 (`godot-mono`), C# on `net8.0` via `Godot.NET.Sdk/4.7.2`, CsvHelper 33.1.0, xUnit on `net10.0`, Python 3 (stdlib only) for the reference check.

**Spec:** `docs/superpowers/specs/2026-09-05-hayes-pose-projection-design.md`

## Global Constraints

- Godot binary is `godot-mono`. The plain `godot` binary has no C# support and must not be used.
- Main project targets `net8.0` with `Godot.NET.Sdk/4.7.2` and `<EnableDynamicLoading>true</EnableDynamicLoading>`.
- Test project targets `net10.0` with `<RollForward>LatestMajor</RollForward>`. Only the .NET 10.0.11 runtime is installed; a `net8.0` test host will not start.
- Viewport is locked to exactly 1000×1000: `viewport_width=1000`, `viewport_height=1000`, `resizable=false`, `stretch/mode="canvas_items"`, `stretch/aspect="keep"`. Never change these — `unproject_position` returning literal image pixels depends on them.
- Focal length `f = 1148.6` px, principal point `(500, 500)`, `ChallengeCam.Fov = 47.048195°`, `KeepAspect = KeepAspectEnum.Height`.
- World data is Z-up millimetres. Godot conversion is `new Vector3(x, z, -y) / 1000f`.
- Keyboard is `W A S D Q E`, `Shift`, `Esc` only. Every other operation is a UI control. Do not add other shortcut keys.
- Style: plain C#. `for` loops, not LINQ. Explicit types, not `var` chains. No records, no pattern matching, no expression-bodied members. Comment only where the geometry is not self-evident.
- Bone colours: right side red, left side blue, spine black.

---

### Task 1: Project skeleton, data, and both build harnesses

**Files:**
- Create: `project.godot`, `PoseProjection.csproj`, `.gitignore`, `scenes/Main.tscn`, `scripts/Main.cs`
- Create: `data/focal.txt`, `data/joint-names.txt`, `data/poses.txt`, `data/frames/00.png` … `19.png`
- Create: `tests/.gdignore`, `tests/PoseProjection.Tests/PoseProjection.Tests.csproj`
- Test: `tests/PoseProjection.Tests/DataFilesTests.cs`

**Interfaces:**
- Consumes: nothing.
- Produces: a buildable Godot C# project; `data/` at a known path; `dotnet test` runnable from `tests/PoseProjection.Tests/`.

- [ ] **Step 1: Fetch and place the challenge data**

```bash
cd "$(git rev-parse --show-toplevel)"
mkdir -p data
curl -sL -o /tmp/Pose.zip https://ics.uci.edu/~wayne/research/students/Pose.zip
unzip -q -o /tmp/Pose.zip -d /tmp/posezip
cp /tmp/posezip/Pose/focal.txt /tmp/posezip/Pose/joint-names.txt /tmp/posezip/Pose/poses.txt data/
cp -r /tmp/posezip/Pose/frames data/frames
cp /tmp/posezip/Pose/pose-sample.png docs/pose-sample.png
ls data/frames | wc -l   # expect 20
```

- [ ] **Step 2: Write the failing test**

Create `tests/PoseProjection.Tests/DataFilesTests.cs`:

```csharp
using System;
using System.IO;
using Xunit;

public class DataFilesTests
{
	public static string DataDir()
	{
		string dir = AppContext.BaseDirectory;
		for (int i = 0; i < 8; i++)
		{
			string candidate = Path.Combine(dir, "data");
			if (Directory.Exists(candidate))
			{
				return candidate;
			}
			dir = Path.GetFullPath(Path.Combine(dir, ".."));
		}
		throw new DirectoryNotFoundException("could not locate the data directory");
	}

	[Fact]
	public void PosesFileHasTwentyRowsOfFortyFiveColumns()
	{
		string[] lines = File.ReadAllLines(Path.Combine(DataDir(), "poses.txt"));
		Assert.Equal(20, lines.Length);
		for (int i = 0; i < lines.Length; i++)
		{
			string[] parts = lines[i].Split('\t', StringSplitOptions.RemoveEmptyEntries);
			Assert.Equal(45, parts.Length);
		}
	}

	[Fact]
	public void FocalFileHoldsTheExpectedValue()
	{
		string text = File.ReadAllText(Path.Combine(DataDir(), "focal.txt")).Trim();
		Assert.Equal("1148.6", text);
	}

	[Fact]
	public void ThereAreTwentyFrameImages()
	{
		string[] files = Directory.GetFiles(Path.Combine(DataDir(), "frames"), "*.png");
		Assert.Equal(20, files.Length);
	}
}
```

- [ ] **Step 3: Create the test project**

Create `tests/PoseProjection.Tests/PoseProjection.Tests.csproj`:

```xml
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net10.0</TargetFramework>
    <RollForward>LatestMajor</RollForward>
    <Nullable>disable</Nullable>
    <IsPackable>false</IsPackable>
    <EnableDefaultCompileItems>false</EnableDefaultCompileItems>
  </PropertyGroup>
  <ItemGroup>
    <Compile Include="*.cs" />
  </ItemGroup>
  <ItemGroup>
    <PackageReference Include="Microsoft.NET.Test.Sdk" Version="17.11.1" />
    <PackageReference Include="xunit" Version="2.9.2" />
    <PackageReference Include="xunit.runner.visualstudio" Version="2.8.2" />
    <PackageReference Include="CsvHelper" Version="33.1.0" />
  </ItemGroup>
  <ItemGroup>
    <Reference Include="GodotSharp">
      <HintPath>/usr/lib/godot-mono/GodotSharp/Api/Release/GodotSharp.dll</HintPath>
    </Reference>
  </ItemGroup>
</Project>
```

`EnableDefaultCompileItems` is off and sources are listed explicitly, because later tasks add `<Compile Include="../../scripts/X.cs" />` lines one at a time — only the engine-free scripts ever get added.

Create `tests/.gdignore` as an empty file so the Godot editor does not import the test project.

- [ ] **Step 4: Run the test to verify it fails**

Run: `cd tests/PoseProjection.Tests && dotnet test --nologo`
Expected: FAIL if the data step was skipped. If Step 1 was done, these three pass immediately — that is fine, they are a regression guard on the committed data, not a design driver.

- [ ] **Step 5: Create the Godot project files**

Create `project.godot`:

```
config_version=5

[application]

config/name="Hayes Pose Projection"
run/main_scene="res://scenes/Main.tscn"
config/features=PackedStringArray("4.4", "C#")

[display]

window/size/viewport_width=1000
window/size/viewport_height=1000
window/size/resizable=false
window/stretch/mode="canvas_items"
window/stretch/aspect="keep"

[dotnet]

project/assembly_name="PoseProjection"
```

Create `PoseProjection.csproj`:

```xml
<Project Sdk="Godot.NET.Sdk/4.7.2">
  <PropertyGroup>
    <TargetFramework>net8.0</TargetFramework>
    <EnableDynamicLoading>true</EnableDynamicLoading>
    <RootNamespace>PoseProjection</RootNamespace>
  </PropertyGroup>
  <ItemGroup>
    <Compile Remove="tests/**/*.cs" />
    <PackageReference Include="CsvHelper" Version="33.1.0" />
  </ItemGroup>
</Project>
```

The `Compile Remove` is required: `Godot.NET.Sdk` globs `**/*.cs`, so without it the test sources are pulled into the game assembly and the build fails on the missing xUnit reference.

Create `.gitignore`:

```
.godot/
bin/
obj/
out/
*.user
```

- [ ] **Step 6: Create the minimal main scene**

Create `scripts/Main.cs`:

```csharp
using Godot;

public partial class Main : Node
{
	public override void _Ready()
	{
		GD.Print("pose projection: ready");
	}
}
```

Create `scenes/Main.tscn`:

```
[gd_scene load_steps=2 format=3]

[ext_resource type="Script" path="res://scripts/Main.cs" id="1"]

[node name="Main" type="Node"]
script = ExtResource("1")
```

- [ ] **Step 7: Verify both builds and the engine run**

```bash
cd "$(git rev-parse --show-toplevel)"
dotnet build PoseProjection.csproj --nologo          # expect: Build succeeded, 0 errors
(cd tests/PoseProjection.Tests && dotnet test --nologo)   # expect: Passed! 3 tests
timeout 120 godot-mono --headless --path . --quit-after 20
```

Expected from the last command: `pose projection: ready`. If instead you see `No loader found for resource: res://scripts/Main.cs`, you ran the wrong binary — use `godot-mono`.

This run also imports the 20 PNGs into `.godot/imported/`, which is what makes `GD.Load<Texture2D>("res://data/frames/NN.png")` work in Task 9. `.godot/` is gitignored, so a fresh clone needs one such run before the photograph background will load.

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "Add Godot C# project skeleton, challenge data, and test harness"
```

---

### Task 2: Parse the challenge data

**Files:**
- Create: `scripts/PoseFrame.cs`, `scripts/PoseData.cs`
- Modify: `tests/PoseProjection.Tests/PoseProjection.Tests.csproj` (add two `Compile Include` lines)
- Test: `tests/PoseProjection.Tests/PoseDataTests.cs`

**Interfaces:**
- Consumes: `DataFilesTests.DataDir()` from Task 1.
- Produces:
  - `PoseFrame` with fields `Vector3 CameraWorld`, `Vector3 CameraGodot`, `Vector3[] JointsWorld` (14), `Vector3[] JointsGodot` (14), `Vector3 CentroidWorld`, `Vector3 CentroidGodot`; and `static Vector3 PoseFrame.ToGodot(Vector3 world)`.
  - `PoseData` with fields `float Focal`, `string[] JointNames` (14), `PoseFrame[] Frames` (20); and `static PoseData Parse(TextReader posesReader, TextReader jointNamesReader, string focalText)`.

- [ ] **Step 1: Write the failing test**

Create `tests/PoseProjection.Tests/PoseDataTests.cs`:

```csharp
using System.IO;
using Godot;
using Xunit;

public class PoseDataTests
{
	public static PoseData Load()
	{
		string dir = DataFilesTests.DataDir();
		StreamReader poses = new StreamReader(Path.Combine(dir, "poses.txt"));
		StreamReader names = new StreamReader(Path.Combine(dir, "joint-names.txt"));
		string focal = File.ReadAllText(Path.Combine(dir, "focal.txt"));
		PoseData data = PoseData.Parse(poses, names, focal);
		poses.Dispose();
		names.Dispose();
		return data;
	}

	[Fact]
	public void ReadsFocalAndCounts()
	{
		PoseData data = Load();
		Assert.Equal(1148.6f, data.Focal, 3);
		Assert.Equal(20, data.Frames.Length);
		Assert.Equal(14, data.JointNames.Length);
	}

	[Fact]
	public void ReadsJointNamesWithoutQuotes()
	{
		PoseData data = Load();
		Assert.Equal("Hip", data.JointNames[0]);
		Assert.Equal("RAnkle", data.JointNames[3]);
		Assert.Equal("Neck", data.JointNames[7]);
		Assert.Equal("RWrist", data.JointNames[13]);
	}

	[Fact]
	public void ReadsFirstFrameCameraAndJoints()
	{
		PoseData data = Load();
		PoseFrame frame = data.Frames[0];
		Assert.Equal(1761.27853428116f, frame.CameraWorld.X, 2);
		Assert.Equal(-5078.00659454077f, frame.CameraWorld.Y, 2);
		Assert.Equal(1606.2649598335f, frame.CameraWorld.Z, 2);

		Assert.Equal(14, frame.JointsWorld.Length);
		Assert.Equal(9.433470f, frame.JointsWorld[0].X, 3);
		Assert.Equal(-15.833000f, frame.JointsWorld[0].Y, 3);
		Assert.Equal(158.005997f, frame.JointsWorld[0].Z, 3);

		Assert.Equal(16.278038f, frame.JointsWorld[7].X, 3);
		Assert.Equal(-65.465576f, frame.JointsWorld[7].Y, 3);
		Assert.Equal(630.855408f, frame.JointsWorld[7].Z, 3);
	}

	[Fact]
	public void ConvertsToGodotSpaceInMetres()
	{
		PoseData data = Load();
		PoseFrame frame = data.Frames[0];
		// (x, z, -y) / 1000
		Assert.Equal(0.00943347f, frame.JointsGodot[0].X, 6);
		Assert.Equal(0.15800600f, frame.JointsGodot[0].Y, 6);
		Assert.Equal(0.01583300f, frame.JointsGodot[0].Z, 6);

		Assert.Equal(1.76127853f, frame.CameraGodot.X, 6);
		Assert.Equal(1.60626496f, frame.CameraGodot.Y, 6);
		Assert.Equal(5.07800659f, frame.CameraGodot.Z, 6);
	}

	[Fact]
	public void ConversionPreservesHandedness()
	{
		Vector3 x = PoseFrame.ToGodot(new Vector3(1000, 0, 0));
		Vector3 y = PoseFrame.ToGodot(new Vector3(0, 1000, 0));
		Vector3 z = PoseFrame.ToGodot(new Vector3(0, 0, 1000));
		// determinant of the mapped basis must be +1, not -1
		float det = x.Dot(y.Cross(z));
		Assert.Equal(1.0f, det, 5);
	}

	[Fact]
	public void ComputesTheCentroidOfTheFourteenJoints()
	{
		PoseData data = Load();
		PoseFrame frame = data.Frames[0];
		Assert.Equal(30.35914f, frame.CentroidWorld.X, 3);
		Assert.Equal(-110.13630f, frame.CentroidWorld.Y, 3);
		Assert.Equal(269.79306f, frame.CentroidWorld.Z, 3);
	}

	[Fact]
	public void EveryFrameHasFourteenJoints()
	{
		PoseData data = Load();
		for (int i = 0; i < data.Frames.Length; i++)
		{
			Assert.Equal(14, data.Frames[i].JointsWorld.Length);
			Assert.Equal(14, data.Frames[i].JointsGodot.Length);
		}
	}
}
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `cd tests/PoseProjection.Tests && dotnet test --nologo`
Expected: FAIL — `PoseData` and `PoseFrame` do not exist (CS0246).

- [ ] **Step 3: Write `PoseFrame`**

Create `scripts/PoseFrame.cs`:

```csharp
using Godot;

// One row of poses.txt: a camera position and 14 joint positions.
// Kept in both the source coordinate system and Godot's, because the
// manual projector works from the raw values and the scene works from
// the converted ones.
public class PoseFrame
{
	public Vector3 CameraWorld;    // millimetres, Z-up
	public Vector3 CameraGodot;    // metres, Y-up
	public Vector3[] JointsWorld;  // 14 joints, millimetres, Z-up
	public Vector3[] JointsGodot;  // 14 joints, metres, Y-up
	public Vector3 CentroidWorld;
	public Vector3 CentroidGodot;

	// The source data is Z-up in millimetres; Godot is Y-up in metres.
	// Mapping (x, y, z) to (x, z, -y) keeps the system right-handed.
	public static Vector3 ToGodot(Vector3 world)
	{
		return new Vector3(world.X, world.Z, -world.Y) / 1000.0f;
	}

	public void ComputeDerived()
	{
		Vector3 sum = Vector3.Zero;
		for (int i = 0; i < JointsWorld.Length; i++)
		{
			sum += JointsWorld[i];
		}
		CentroidWorld = sum / JointsWorld.Length;
		CentroidGodot = ToGodot(CentroidWorld);

		CameraGodot = ToGodot(CameraWorld);
		JointsGodot = new Vector3[JointsWorld.Length];
		for (int i = 0; i < JointsWorld.Length; i++)
		{
			JointsGodot[i] = ToGodot(JointsWorld[i]);
		}
	}
}
```

- [ ] **Step 4: Write `PoseData`**

Create `scripts/PoseData.cs`:

```csharp
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using CsvHelper;
using CsvHelper.Configuration;
using Godot;

// Parses the three text files from Pose.zip. Takes readers rather than
// paths so it can be tested without the engine's res:// filesystem.
public class PoseData
{
	public const int JointCount = 14;

	public float Focal;
	public string[] JointNames;
	public PoseFrame[] Frames;

	public static PoseData Parse(TextReader posesReader, TextReader jointNamesReader, string focalText)
	{
		PoseData data = new PoseData();
		data.Focal = float.Parse(focalText.Trim(), CultureInfo.InvariantCulture);
		data.JointNames = ParseJointNames(jointNamesReader);
		data.Frames = ParsePoses(posesReader);
		return data;
	}

	private static string[] ParseJointNames(TextReader reader)
	{
		List<string> names = new List<string>();
		string line = reader.ReadLine();
		while (line != null)
		{
			string trimmed = line.Trim();
			if (trimmed.Length > 0)
			{
				// each line is "0   'Hip'"
				int quote = trimmed.IndexOf('\'');
				string name = trimmed.Substring(quote + 1).TrimEnd('\'');
				names.Add(name);
			}
			line = reader.ReadLine();
		}
		return names.ToArray();
	}

	private static PoseFrame[] ParsePoses(TextReader reader)
	{
		CsvConfiguration config = new CsvConfiguration(CultureInfo.InvariantCulture);
		config.Delimiter = "\t";
		config.HasHeaderRecord = false;

		List<PoseFrame> frames = new List<PoseFrame>();
		CsvReader csv = new CsvReader(reader, config);
		while (csv.Read())
		{
			float[] values = new float[3 + JointCount * 3];
			for (int i = 0; i < values.Length; i++)
			{
				values[i] = csv.GetField<float>(i);
			}

			PoseFrame frame = new PoseFrame();
			frame.CameraWorld = new Vector3(values[0], values[1], values[2]);
			frame.JointsWorld = new Vector3[JointCount];
			for (int j = 0; j < JointCount; j++)
			{
				int at = 3 + j * 3;
				frame.JointsWorld[j] = new Vector3(values[at], values[at + 1], values[at + 2]);
			}
			frame.ComputeDerived();
			frames.Add(frame);
		}
		return frames.ToArray();
	}
}
```

- [ ] **Step 5: Add the sources to the test project**

In `tests/PoseProjection.Tests/PoseProjection.Tests.csproj`, replace the compile item group with:

```xml
  <ItemGroup>
    <Compile Include="*.cs" />
    <Compile Include="../../scripts/PoseFrame.cs" />
    <Compile Include="../../scripts/PoseData.cs" />
  </ItemGroup>
```

- [ ] **Step 6: Run the tests to verify they pass**

Run: `cd tests/PoseProjection.Tests && dotnet test --nologo`
Expected: PASS, 10 tests total.

Also confirm the game assembly still builds, since `scripts/` is now non-trivial:
Run: `dotnet build PoseProjection.csproj --nologo` — expect 0 errors.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "Parse poses.txt, joint-names.txt and focal.txt into PoseData"
```

---

### Task 3: The bone table

**Files:**
- Create: `scripts/Bones.cs`
- Modify: `tests/PoseProjection.Tests/PoseProjection.Tests.csproj` (add one `Compile Include` line)
- Test: `tests/PoseProjection.Tests/BonesTests.cs`

**Interfaces:**
- Consumes: nothing.
- Produces: `enum BoneSide { Right, Left, Spine }`; `class Bone` with `int A`, `int B`, `BoneSide Side`; `static class Bones` with `static readonly Bone[] All` (13 entries) and `static Color ColorFor(BoneSide side)`.

- [ ] **Step 1: Write the failing test**

Create `tests/PoseProjection.Tests/BonesTests.cs`:

```csharp
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
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `cd tests/PoseProjection.Tests && dotnet test --nologo`
Expected: FAIL — `Bones` does not exist (CS0246).

- [ ] **Step 3: Write `Bones`**

Create `scripts/Bones.cs`:

```csharp
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
```

- [ ] **Step 4: Add the source to the test project**

Add `<Compile Include="../../scripts/Bones.cs" />` to the compile item group.

- [ ] **Step 5: Run the tests to verify they pass**

Run: `cd tests/PoseProjection.Tests && dotnet test --nologo`
Expected: PASS, 16 tests total.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "Add the 13-bone skeleton table with side colours"
```

---

### Task 4: The manual pinhole projector

This is the task that answers challenge parts (a) and (b). The expected values below were computed independently in Python during design; they are the ground truth for the whole project.

**Files:**
- Create: `scripts/IProjector.cs`, `scripts/ManualProjector.cs`
- Modify: `tests/PoseProjection.Tests/PoseProjection.Tests.csproj` (add two `Compile Include` lines)
- Test: `tests/PoseProjection.Tests/ManualProjectorTests.cs`

**Interfaces:**
- Consumes: `PoseData`, `PoseFrame` from Task 2.
- Produces:
  - `enum ProjectionMethod { ManualPinhole, GodotUnproject }`
  - `interface IProjector` with `void Begin(PoseFrame frame, float focal)` and `Vector2 Project(int jointIndex)`
  - `class ManualProjector : IProjector` with a parameterless constructor
  - `const float ManualProjector.PrincipalPoint = 500.0f`

- [ ] **Step 1: Write the failing test**

Create `tests/PoseProjection.Tests/ManualProjectorTests.cs`:

```csharp
using Godot;
using Xunit;

public class ManualProjectorTests
{
	// Ground truth from the independent Python implementation, frame 00.
	private static readonly float[,] Frame00 = new float[,]
	{
		{ 502.333f, 517.539f }, { 480.252f, 503.825f }, { 485.110f, 497.054f },
		{ 428.339f, 577.699f }, { 524.751f, 531.463f }, { 500.437f, 541.094f },
		{ 462.049f, 553.164f }, { 500.299f, 422.890f }, { 530.248f, 433.410f },
		{ 563.850f, 478.121f }, { 581.759f, 527.989f }, { 478.020f, 434.501f },
		{ 483.533f, 470.911f }, { 475.622f, 516.824f }
	};

	// Ground truth for frame 19, whose camera is on the opposite side of the room.
	private static readonly float[,] Frame19 = new float[,]
	{
		{ 501.860f, 508.145f }, { 527.319f, 506.708f }, { 533.577f, 557.377f },
		{ 510.035f, 543.297f }, { 476.692f, 509.565f }, { 499.567f, 554.163f },
		{ 479.916f, 539.331f }, { 498.953f, 412.108f }, { 475.023f, 426.704f },
		{ 448.962f, 474.435f }, { 434.662f, 520.478f }, { 523.354f, 428.259f },
		{ 540.535f, 480.548f }, { 552.195f, 530.374f }
	};

	[Fact]
	public void ProjectsFrameZeroToTheKnownPixelCoordinates()
	{
		AssertFrameMatches(0, Frame00);
	}

	[Fact]
	public void ProjectsFrameNineteenToTheKnownPixelCoordinates()
	{
		AssertFrameMatches(19, Frame19);
	}

	[Fact]
	public void EveryJointOfEveryFrameLandsInsideTheImage()
	{
		PoseData data = PoseDataTests.Load();
		ManualProjector projector = new ManualProjector();
		for (int f = 0; f < data.Frames.Length; f++)
		{
			projector.Begin(data.Frames[f], data.Focal);
			for (int j = 0; j < PoseData.JointCount; j++)
			{
				Vector2 p = projector.Project(j);
				Assert.InRange(p.X, 0.0f, 1000.0f);
				Assert.InRange(p.Y, 0.0f, 1000.0f);
			}
		}
	}

	[Fact]
	public void TheNeckProjectsAboveTheAnklesInEveryFrame()
	{
		// v grows downward, so a correct orientation puts the neck at a
		// smaller v than both ankles. This catches an inverted up vector.
		PoseData data = PoseDataTests.Load();
		ManualProjector projector = new ManualProjector();
		for (int f = 0; f < data.Frames.Length; f++)
		{
			projector.Begin(data.Frames[f], data.Focal);
			float neck = projector.Project(7).Y;
			Assert.True(neck < projector.Project(3).Y, "frame " + f + " right ankle");
			Assert.True(neck < projector.Project(6).Y, "frame " + f + " left ankle");
		}
	}

	private static void AssertFrameMatches(int frameIndex, float[,] expected)
	{
		PoseData data = PoseDataTests.Load();
		ManualProjector projector = new ManualProjector();
		projector.Begin(data.Frames[frameIndex], data.Focal);
		for (int j = 0; j < PoseData.JointCount; j++)
		{
			Vector2 p = projector.Project(j);
			Assert.Equal(expected[j, 0], p.X, 2);
			Assert.Equal(expected[j, 1], p.Y, 2);
		}
	}
}
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `cd tests/PoseProjection.Tests && dotnet test --nologo`
Expected: FAIL — `ManualProjector` does not exist (CS0246).

- [ ] **Step 3: Write the interface**

Create `scripts/IProjector.cs`:

```csharp
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
```

- [ ] **Step 4: Write `ManualProjector`**

Create `scripts/ManualProjector.cs`:

```csharp
using Godot;

// Textbook pinhole projection, done by hand from the raw millimetre data.
// It never touches the scene graph or a Camera3D, so it is a genuinely
// independent check on the Godot implementation.
public class ManualProjector : IProjector
{
	// The frames are 1000x1000, so the principal point is the centre.
	public const float PrincipalPoint = 500.0f;

	private PoseFrame _frame;
	private float _focal;
	private Vector3 _forward;
	private Vector3 _right;
	private Vector3 _up;

	public void Begin(PoseFrame frame, float focal)
	{
		_frame = frame;
		_focal = focal;

		// Challenge part (a): point the camera at the subject. The centroid
		// of the joints is used rather than the hip so crouched and reaching
		// poses stay framed.
		_forward = (frame.CentroidWorld - frame.CameraWorld).Normalized();

		// World up is +Z here, not +Y. Crossing it out of forward gives a
		// camera with no roll.
		Vector3 worldUp = new Vector3(0.0f, 0.0f, 1.0f);
		_right = _forward.Cross(worldUp).Normalized();
		_up = _right.Cross(_forward);
	}

	public Vector2 Project(int jointIndex)
	{
		Vector3 d = _frame.JointsWorld[jointIndex] - _frame.CameraWorld;
		float x = d.Dot(_right);
		float y = d.Dot(_up);
		float z = d.Dot(_forward);

		float u = _focal * x / z + PrincipalPoint;
		// Image rows count downward while _up points up, so v is subtracted.
		float v = PrincipalPoint - _focal * y / z;
		return new Vector2(u, v);
	}
}
```

- [ ] **Step 5: Add the sources to the test project**

Add `<Compile Include="../../scripts/IProjector.cs" />` and `<Compile Include="../../scripts/ManualProjector.cs" />` to the compile item group.

- [ ] **Step 6: Run the tests to verify they pass**

Run: `cd tests/PoseProjection.Tests && dotnet test --nologo`
Expected: PASS, 20 tests total.

If the frame-00 assertions fail with u and v swapped or mirrored, the cross-product order in `Begin` is wrong — `_right` must be `forward × worldUp`, not `worldUp × forward`.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "Add the hand-rolled pinhole projector with golden-value tests"
```

---

### Task 5: 2D view framing

**Files:**
- Create: `scripts/Framing.cs`
- Modify: `tests/PoseProjection.Tests/PoseProjection.Tests.csproj` (add one `Compile Include` line)
- Test: `tests/PoseProjection.Tests/FramingTests.cs`

**Interfaces:**
- Consumes: nothing.
- Produces:
  - `enum ViewFraming { FitToPose, ImagePixels }`
  - `struct FramingTransform` with `float Scale`, `Vector2 Offset`, and `Vector2 Apply(Vector2 p)`
  - `static class Framing` with `static FramingTransform Compute(Vector2[] points, ViewFraming mode, float viewportSize, float padFraction)`

- [ ] **Step 1: Write the failing test**

Create `tests/PoseProjection.Tests/FramingTests.cs`:

```csharp
using Godot;
using Xunit;

public class FramingTests
{
	private static Vector2[] Box()
	{
		return new Vector2[]
		{
			new Vector2(400.0f, 400.0f),
			new Vector2(600.0f, 400.0f),
			new Vector2(400.0f, 500.0f),
			new Vector2(600.0f, 500.0f)
		};
	}

	[Fact]
	public void ImagePixelsFramingIsTheIdentity()
	{
		FramingTransform t = Framing.Compute(Box(), ViewFraming.ImagePixels, 1000.0f, 0.1f);
		Assert.Equal(1.0f, t.Scale, 6);
		Assert.Equal(0.0f, t.Offset.X, 6);
		Assert.Equal(0.0f, t.Offset.Y, 6);
		Assert.Equal(new Vector2(400.0f, 400.0f), t.Apply(new Vector2(400.0f, 400.0f)));
	}

	[Fact]
	public void FitToPoseScalesTheLongerSideToTheViewportLessPadding()
	{
		// bounding box is 200 wide by 100 tall; padded by 10% of 200 on each
		// side gives 240; 1000 / 240 scales it up.
		FramingTransform t = Framing.Compute(Box(), ViewFraming.FitToPose, 1000.0f, 0.1f);
		Assert.Equal(1000.0f / 240.0f, t.Scale, 4);
	}

	[Fact]
	public void FitToPoseCentresTheBoundingBoxInTheViewport()
	{
		FramingTransform t = Framing.Compute(Box(), ViewFraming.FitToPose, 1000.0f, 0.1f);
		Vector2 centre = t.Apply(new Vector2(500.0f, 450.0f));
		Assert.Equal(500.0f, centre.X, 3);
		Assert.Equal(500.0f, centre.Y, 3);
	}

	[Fact]
	public void FitToPoseKeepsEveryPointInsideTheViewport()
	{
		FramingTransform t = Framing.Compute(Box(), ViewFraming.FitToPose, 1000.0f, 0.1f);
		Vector2[] points = Box();
		for (int i = 0; i < points.Length; i++)
		{
			Vector2 p = t.Apply(points[i]);
			Assert.InRange(p.X, 0.0f, 1000.0f);
			Assert.InRange(p.Y, 0.0f, 1000.0f);
		}
	}

	[Fact]
	public void FitToPoseIsUniformSoThePoseIsNotStretched()
	{
		// A wide, flat pose must keep its aspect ratio: the horizontal and
		// vertical gaps scale by the same factor.
		FramingTransform t = Framing.Compute(Box(), ViewFraming.FitToPose, 1000.0f, 0.1f);
		Vector2 a = t.Apply(new Vector2(400.0f, 400.0f));
		Vector2 b = t.Apply(new Vector2(600.0f, 500.0f));
		float sx = (b.X - a.X) / 200.0f;
		float sy = (b.Y - a.Y) / 100.0f;
		Assert.Equal(sx, sy, 5);
	}

	[Fact]
	public void ADegeneratePoseDoesNotDivideByZero()
	{
		Vector2[] single = new Vector2[] { new Vector2(500.0f, 500.0f) };
		FramingTransform t = Framing.Compute(single, ViewFraming.FitToPose, 1000.0f, 0.1f);
		Vector2 p = t.Apply(single[0]);
		Assert.False(float.IsNaN(p.X));
		Assert.False(float.IsNaN(p.Y));
	}
}
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `cd tests/PoseProjection.Tests && dotnet test --nologo`
Expected: FAIL — `Framing` does not exist (CS0246).

- [ ] **Step 3: Write `Framing`**

Create `scripts/Framing.cs`:

```csharp
using Godot;

public enum ViewFraming
{
	FitToPose,
	ImagePixels
}

public struct FramingTransform
{
	public float Scale;
	public Vector2 Offset;

	public Vector2 Apply(Vector2 p)
	{
		return p * Scale + Offset;
	}
}

// A projected figure only spans about 100-300 px of the 1000x1000 frame.
// That is correct over the photograph but unreadable on a white background,
// so the white view scales the pose up to fill the viewport. This only
// affects what is drawn; recorded coordinates stay in image pixels.
public static class Framing
{
	public static FramingTransform Compute(Vector2[] points, ViewFraming mode, float viewportSize, float padFraction)
	{
		FramingTransform t = new FramingTransform();
		if (mode == ViewFraming.ImagePixels || points.Length == 0)
		{
			t.Scale = 1.0f;
			t.Offset = Vector2.Zero;
			return t;
		}

		float minX = points[0].X;
		float maxX = points[0].X;
		float minY = points[0].Y;
		float maxY = points[0].Y;
		for (int i = 1; i < points.Length; i++)
		{
			minX = Mathf.Min(minX, points[i].X);
			maxX = Mathf.Max(maxX, points[i].X);
			minY = Mathf.Min(minY, points[i].Y);
			maxY = Mathf.Max(maxY, points[i].Y);
		}

		float span = Mathf.Max(maxX - minX, maxY - minY);
		if (span < 0.001f)
		{
			span = 1.0f;
		}
		float padded = span * (1.0f + 2.0f * padFraction);

		t.Scale = viewportSize / padded;
		Vector2 centre = new Vector2((minX + maxX) * 0.5f, (minY + maxY) * 0.5f);
		t.Offset = new Vector2(viewportSize * 0.5f, viewportSize * 0.5f) - centre * t.Scale;
		return t;
	}
}
```

- [ ] **Step 4: Add the source to the test project**

Add `<Compile Include="../../scripts/Framing.cs" />` to the compile item group.

- [ ] **Step 5: Run the tests to verify they pass**

Run: `cd tests/PoseProjection.Tests && dotnet test --nologo`
Expected: PASS, 26 tests total.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "Add fit-to-pose and image-pixel framing for the 2D view"
```

---

### Task 6: The coordinate tables

**Files:**
- Create: `scripts/CoordinateTable.cs`
- Modify: `tests/PoseProjection.Tests/PoseProjection.Tests.csproj` (add one `Compile Include` line)
- Test: `tests/PoseProjection.Tests/CoordinateTableTests.cs`

**Interfaces:**
- Consumes: `PoseData.JointCount` from Task 2.
- Produces:
  - `class ProjectedFrame` with `int Frame` and `Vector2[] Points` (14)
  - `static class CoordinateTable` with `static void WriteWide(TextWriter w, ProjectedFrame[] frames)`, `static void WriteLong(TextWriter w, ProjectedFrame[] manual, ProjectedFrame[] godot, string[] jointNames)`, `static void WriteAgreement(TextWriter w, ProjectedFrame[] manual, ProjectedFrame[] godot)`, `static float MaxAbsDiff(ProjectedFrame a, ProjectedFrame b)`, `static float MeanAbsDiff(ProjectedFrame a, ProjectedFrame b)`

- [ ] **Step 1: Write the failing test**

Create `tests/PoseProjection.Tests/CoordinateTableTests.cs`:

```csharp
using System.IO;
using Godot;
using Xunit;

public class CoordinateTableTests
{
	private static ProjectedFrame MakeFrame(int index, float bias)
	{
		ProjectedFrame f = new ProjectedFrame();
		f.Frame = index;
		f.Points = new Vector2[PoseData.JointCount];
		for (int j = 0; j < f.Points.Length; j++)
		{
			f.Points[j] = new Vector2(100.0f + j + bias, 200.0f + j + bias);
		}
		return f;
	}

	[Fact]
	public void WideTableHasAHeaderAndOneRowPerFrame()
	{
		ProjectedFrame[] frames = new ProjectedFrame[] { MakeFrame(0, 0.0f), MakeFrame(1, 0.0f) };
		StringWriter writer = new StringWriter();
		CoordinateTable.WriteWide(writer, frames);
		string[] lines = writer.ToString().Trim().Split('\n');

		Assert.Equal(3, lines.Length);
		string[] header = lines[0].Trim().Split(',');
		Assert.Equal(29, header.Length);
		Assert.Equal("frame", header[0]);
		Assert.Equal("u0", header[1]);
		Assert.Equal("v0", header[2]);
		Assert.Equal("u13", header[27]);
		Assert.Equal("v13", header[28]);

		string[] row = lines[1].Trim().Split(',');
		Assert.Equal(29, row.Length);
		Assert.Equal("0", row[0]);
	}

	[Fact]
	public void WideTableWritesThreeDecimalsInInvariantCulture()
	{
		ProjectedFrame[] frames = new ProjectedFrame[] { MakeFrame(0, 0.5f) };
		StringWriter writer = new StringWriter();
		CoordinateTable.WriteWide(writer, frames);
		string[] row = writer.ToString().Trim().Split('\n')[1].Trim().Split(',');
		Assert.Equal("100.500", row[1]);
		Assert.Equal("200.500", row[2]);
	}

	[Fact]
	public void LongTableHasOneRowPerJointPerMethod()
	{
		ProjectedFrame[] manual = new ProjectedFrame[] { MakeFrame(0, 0.0f), MakeFrame(1, 0.0f) };
		ProjectedFrame[] godot = new ProjectedFrame[] { MakeFrame(0, 0.0f), MakeFrame(1, 0.0f) };
		string[] names = new string[PoseData.JointCount];
		for (int j = 0; j < names.Length; j++)
		{
			names[j] = "J" + j;
		}

		StringWriter writer = new StringWriter();
		CoordinateTable.WriteLong(writer, manual, godot, names);
		string[] lines = writer.ToString().Trim().Split('\n');

		// header + 2 frames * 14 joints * 2 methods
		Assert.Equal(1 + 2 * 14 * 2, lines.Length);
		Assert.Equal("frame,joint_id,joint_name,method,u,v", lines[0].Trim());
	}

	[Fact]
	public void DiffsAreZeroForIdenticalFrames()
	{
		ProjectedFrame a = MakeFrame(0, 0.0f);
		ProjectedFrame b = MakeFrame(0, 0.0f);
		Assert.Equal(0.0f, CoordinateTable.MaxAbsDiff(a, b), 6);
		Assert.Equal(0.0f, CoordinateTable.MeanAbsDiff(a, b), 6);
	}

	[Fact]
	public void DiffsMeasureTheLargestSingleAxisDeviation()
	{
		ProjectedFrame a = MakeFrame(0, 0.0f);
		ProjectedFrame b = MakeFrame(0, 0.0f);
		b.Points[5] = new Vector2(b.Points[5].X + 0.25f, b.Points[5].Y);
		Assert.Equal(0.25f, CoordinateTable.MaxAbsDiff(a, b), 5);
		// one axis of one joint out of 28 values
		Assert.Equal(0.25f / 28.0f, CoordinateTable.MeanAbsDiff(a, b), 5);
	}

	[Fact]
	public void AgreementTableHasOneRowPerFrame()
	{
		ProjectedFrame[] manual = new ProjectedFrame[] { MakeFrame(0, 0.0f), MakeFrame(1, 0.0f) };
		ProjectedFrame[] godot = new ProjectedFrame[] { MakeFrame(0, 0.0f), MakeFrame(1, 0.0f) };
		StringWriter writer = new StringWriter();
		CoordinateTable.WriteAgreement(writer, manual, godot);
		string[] lines = writer.ToString().Trim().Split('\n');
		Assert.Equal(3, lines.Length);
		Assert.Equal("frame,max_abs_diff_px,mean_abs_diff_px", lines[0].Trim());
	}
}
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `cd tests/PoseProjection.Tests && dotnet test --nologo`
Expected: FAIL — `CoordinateTable` does not exist (CS0246).

- [ ] **Step 3: Write `CoordinateTable`**

Create `scripts/CoordinateTable.cs`:

```csharp
using System.Globalization;
using System.IO;
using CsvHelper;
using Godot;

public class ProjectedFrame
{
	public int Frame;
	public Vector2[] Points;
}

// The challenge asks for every 2D coordinate in one table. These are always
// true image pixels, never the fit-to-pose display coordinates.
public static class CoordinateTable
{
	private const string Format = "0.000";

	public static void WriteWide(TextWriter writer, ProjectedFrame[] frames)
	{
		CsvWriter csv = new CsvWriter(writer, CultureInfo.InvariantCulture, true);
		csv.WriteField("frame");
		for (int j = 0; j < PoseData.JointCount; j++)
		{
			csv.WriteField("u" + j);
			csv.WriteField("v" + j);
		}
		csv.NextRecord();

		for (int f = 0; f < frames.Length; f++)
		{
			csv.WriteField(frames[f].Frame);
			for (int j = 0; j < PoseData.JointCount; j++)
			{
				csv.WriteField(Text(frames[f].Points[j].X));
				csv.WriteField(Text(frames[f].Points[j].Y));
			}
			csv.NextRecord();
		}
		csv.Flush();
	}

	public static void WriteLong(TextWriter writer, ProjectedFrame[] manual, ProjectedFrame[] godot, string[] jointNames)
	{
		CsvWriter csv = new CsvWriter(writer, CultureInfo.InvariantCulture, true);
		csv.WriteField("frame");
		csv.WriteField("joint_id");
		csv.WriteField("joint_name");
		csv.WriteField("method");
		csv.WriteField("u");
		csv.WriteField("v");
		csv.NextRecord();

		WriteLongRows(csv, manual, jointNames, "manual_pinhole");
		WriteLongRows(csv, godot, jointNames, "godot_unproject");
		csv.Flush();
	}

	private static void WriteLongRows(CsvWriter csv, ProjectedFrame[] frames, string[] jointNames, string method)
	{
		for (int f = 0; f < frames.Length; f++)
		{
			for (int j = 0; j < PoseData.JointCount; j++)
			{
				csv.WriteField(frames[f].Frame);
				csv.WriteField(j);
				csv.WriteField(jointNames[j]);
				csv.WriteField(method);
				csv.WriteField(Text(frames[f].Points[j].X));
				csv.WriteField(Text(frames[f].Points[j].Y));
				csv.NextRecord();
			}
		}
	}

	public static void WriteAgreement(TextWriter writer, ProjectedFrame[] manual, ProjectedFrame[] godot)
	{
		CsvWriter csv = new CsvWriter(writer, CultureInfo.InvariantCulture, true);
		csv.WriteField("frame");
		csv.WriteField("max_abs_diff_px");
		csv.WriteField("mean_abs_diff_px");
		csv.NextRecord();

		for (int f = 0; f < manual.Length; f++)
		{
			csv.WriteField(manual[f].Frame);
			csv.WriteField(MaxAbsDiff(manual[f], godot[f]).ToString("0.000000", CultureInfo.InvariantCulture));
			csv.WriteField(MeanAbsDiff(manual[f], godot[f]).ToString("0.000000", CultureInfo.InvariantCulture));
			csv.NextRecord();
		}
		csv.Flush();
	}

	public static float MaxAbsDiff(ProjectedFrame a, ProjectedFrame b)
	{
		float worst = 0.0f;
		for (int j = 0; j < a.Points.Length; j++)
		{
			worst = Mathf.Max(worst, Mathf.Abs(a.Points[j].X - b.Points[j].X));
			worst = Mathf.Max(worst, Mathf.Abs(a.Points[j].Y - b.Points[j].Y));
		}
		return worst;
	}

	public static float MeanAbsDiff(ProjectedFrame a, ProjectedFrame b)
	{
		float total = 0.0f;
		for (int j = 0; j < a.Points.Length; j++)
		{
			total += Mathf.Abs(a.Points[j].X - b.Points[j].X);
			total += Mathf.Abs(a.Points[j].Y - b.Points[j].Y);
		}
		return total / (a.Points.Length * 2);
	}

	private static string Text(float value)
	{
		return value.ToString(Format, CultureInfo.InvariantCulture);
	}
}
```

- [ ] **Step 4: Add the source to the test project**

Add `<Compile Include="../../scripts/CoordinateTable.cs" />` to the compile item group.

- [ ] **Step 5: Run the tests to verify they pass**

Run: `cd tests/PoseProjection.Tests && dotnet test --nologo`
Expected: PASS, 32 tests total.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "Write the wide, long and agreement coordinate tables with CsvHelper"
```

---

### Task 7: The 3D scene, the challenge camera, and the Godot projector

The two projectors must now agree. The in-engine self-test added here is the check that matters most in the whole project.

**Files:**
- Create: `scripts/PoseLoader.cs`, `scripts/PoseFigure.cs`, `scripts/GodotProjector.cs`, `scripts/SelfTest.cs`
- Modify: `scripts/Main.cs`, `scenes/Main.tscn`

**Interfaces:**
- Consumes: `PoseData`, `PoseFrame`, `Bones`, `IProjector`, `ManualProjector`, `CoordinateTable`, `ProjectedFrame`.
- Produces:
  - `static class PoseLoader` with `static PoseData LoadFromResources()`
  - `partial class PoseFigure : Node3D` with `void Build()`, `void ShowFrame(PoseFrame frame)`, `Node3D GetJointNode(int index)`
  - `class GodotProjector : IProjector` with constructor `GodotProjector(Camera3D camera, PoseFigure figure)`
  - `static class SelfTest` with `static int Run(PoseData data, Camera3D camera, PoseFigure figure)` returning 0 on success
  - `Main` exposes `PoseData Data`, `PoseFigure Figure`, `Camera3D ChallengeCam`, `void AimChallengeCamera(PoseFrame frame)`

- [ ] **Step 1: Write the failing in-engine test**

Create `scripts/SelfTest.cs`:

```csharp
using Godot;

// Runs headlessly with --self-test. Compares the two projectors over every
// joint of every frame. A disagreement means one of them is wrong, and the
// exported table cannot be trusted.
public static class SelfTest
{
	private const float Tolerance = 0.01f;

	public static int Run(PoseData data, Camera3D camera, PoseFigure figure)
	{
		int failures = 0;
		ManualProjector manual = new ManualProjector();
		GodotProjector godot = new GodotProjector(camera, figure);

		for (int f = 0; f < data.Frames.Length; f++)
		{
			PoseFrame frame = data.Frames[f];
			figure.ShowFrame(frame);
			camera.Position = frame.CameraGodot;
			camera.LookAt(frame.CentroidGodot, Vector3.Up);

			manual.Begin(frame, data.Focal);
			godot.Begin(frame, data.Focal);

			for (int j = 0; j < PoseData.JointCount; j++)
			{
				Vector2 a = manual.Project(j);
				Vector2 b = godot.Project(j);
				float dx = Mathf.Abs(a.X - b.X);
				float dy = Mathf.Abs(a.Y - b.Y);
				if (dx > Tolerance || dy > Tolerance)
				{
					GD.PrintErr("frame " + f + " joint " + j + ": manual " + a + " godot " + b);
					failures++;
				}

				if (a.X < 0.0f || a.X > 1000.0f || a.Y < 0.0f || a.Y > 1000.0f)
				{
					GD.PrintErr("frame " + f + " joint " + j + " is outside the image: " + a);
					failures++;
				}
			}
		}

		if (failures == 0)
		{
			GD.Print("self-test: OK, both projectors agree on all 280 points");
			return 0;
		}
		GD.PrintErr("self-test: " + failures + " failures");
		return 1;
	}
}
```

- [ ] **Step 2: Run it to verify it fails**

Run: `dotnet build PoseProjection.csproj --nologo`
Expected: FAIL — `PoseFigure` and `GodotProjector` do not exist (CS0246).

- [ ] **Step 3: Write `PoseLoader`**

Create `scripts/PoseLoader.cs`:

```csharp
using System.IO;
using Godot;

// The engine-side half of loading: reads res:// files and hands plain
// readers to PoseData, which knows nothing about Godot's filesystem.
public static class PoseLoader
{
	public static PoseData LoadFromResources()
	{
		string posesText = ReadText("res://data/poses.txt");
		string namesText = ReadText("res://data/joint-names.txt");
		string focalText = ReadText("res://data/focal.txt");

		StringReader poses = new StringReader(posesText);
		StringReader names = new StringReader(namesText);
		PoseData data = PoseData.Parse(poses, names, focalText);
		poses.Dispose();
		names.Dispose();
		return data;
	}

	private static string ReadText(string path)
	{
		FileAccess file = FileAccess.Open(path, FileAccess.ModeFlags.Read);
		if (file == null)
		{
			GD.PushError("could not open " + path);
			return "";
		}
		string text = file.GetAsText();
		file.Dispose();
		return text;
	}
}
```

- [ ] **Step 4: Write `PoseFigure`**

Create `scripts/PoseFigure.cs`:

```csharp
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
```

- [ ] **Step 5: Write `GodotProjector`**

Create `scripts/GodotProjector.cs`:

```csharp
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
```

- [ ] **Step 6: Wire up `Main`**

Replace `scripts/Main.cs` with:

```csharp
using Godot;

public partial class Main : Node
{
	public const float ImageSize = 1000.0f;

	public PoseData Data;
	public PoseFigure Figure;
	public Camera3D ChallengeCam;

	public override void _Ready()
	{
		Data = PoseLoader.LoadFromResources();

		Figure = new PoseFigure();
		Figure.Name = "PoseFigure";
		AddChild(Figure);
		Figure.Build();

		ChallengeCam = new Camera3D();
		ChallengeCam.Name = "ChallengeCam";
		ChallengeCam.KeepAspect = Camera3D.KeepAspectEnum.Height;
		// The pinhole model has the principal point at the image centre, so
		// half the image height subtends atan(500 / focal).
		ChallengeCam.Fov = Mathf.RadToDeg(2.0f * Mathf.Atan(ImageSize * 0.5f / Data.Focal));
		ChallengeCam.Near = 0.05f;
		AddChild(ChallengeCam);

		Figure.ShowFrame(Data.Frames[0]);
		AimChallengeCamera(Data.Frames[0]);

		string[] args = OS.GetCmdlineUserArgs();
		for (int i = 0; i < args.Length; i++)
		{
			if (args[i] == "--self-test")
			{
				GetTree().Quit(SelfTest.Run(Data, ChallengeCam, Figure));
				return;
			}
		}

		GD.Print("pose projection: " + Data.Frames.Length + " frames, focal " + Data.Focal);
	}

	// Challenge part (a): aim at the subject with no roll.
	public void AimChallengeCamera(PoseFrame frame)
	{
		ChallengeCam.Position = frame.CameraGodot;
		ChallengeCam.LookAt(frame.CentroidGodot, Vector3.Up);
	}
}
```

`scenes/Main.tscn` needs no change — `Main` builds its children in code.

- [ ] **Step 7: Build**

Run: `dotnet build PoseProjection.csproj --nologo`
Expected: Build succeeded, 0 errors.

- [ ] **Step 8: Run the self-test to verify both projectors agree**

```bash
timeout 180 godot-mono --headless --path . -- --self-test
echo "exit code: $?"
```

Expected: `self-test: OK, both projectors agree on all 280 points` and exit code 0.

If instead every point disagrees by a large amount, check that `ChallengeCam.Fov` is 47.048195 and that `KeepAspect` is `Height`. If the disagreement is only in `v`, the world-up handling differs between the two paths.

- [ ] **Step 9: Commit**

```bash
git add -A
git commit -m "Add the 3D figure, challenge camera and Godot projector with a headless self-test"
```

---

### Task 8: The fly camera and the 3D view

**Files:**
- Create: `scripts/FlyCamera.cs`
- Modify: `scripts/Main.cs`

**Interfaces:**
- Consumes: `Main.ChallengeCam`.
- Produces: `partial class FlyCamera : Camera3D` with `void SnapTo(Camera3D other)`; `Main` gains `public FlyCamera Fly` and `public void BuildEnvironment()`.

- [ ] **Step 1: Write `FlyCamera`**

Create `scripts/FlyCamera.cs`:

```csharp
using Godot;

// Free-look camera. The keyboard is deliberately limited to movement:
// everything else in the tool is a UI control.
public partial class FlyCamera : Camera3D
{
	private const float Speed = 3.0f;
	private const float FastSpeed = 9.0f;
	private const float MouseSensitivity = 0.003f;
	private const float PitchLimit = 1.5f;

	private float _yaw;
	private float _pitch;

	public override void _Ready()
	{
		Vector3 angles = Rotation;
		_yaw = angles.Y;
		_pitch = angles.X;
	}

	public override void _UnhandledInput(InputEvent evt)
	{
		InputEventMouseMotion motion = evt as InputEventMouseMotion;
		if (motion != null && Input.MouseMode == Input.MouseModeEnum.Captured)
		{
			_yaw -= motion.Relative.X * MouseSensitivity;
			_pitch -= motion.Relative.Y * MouseSensitivity;
			_pitch = Mathf.Clamp(_pitch, -PitchLimit, PitchLimit);
			Rotation = new Vector3(_pitch, _yaw, 0.0f);
		}

		InputEventMouseButton button = evt as InputEventMouseButton;
		if (button != null && button.Pressed && Current)
		{
			Input.MouseMode = Input.MouseModeEnum.Captured;
		}

		if (evt.IsActionPressed("ui_cancel"))
		{
			Input.MouseMode = Input.MouseModeEnum.Visible;
		}
	}

	public override void _Process(double delta)
	{
		if (!Current)
		{
			return;
		}

		Vector3 move = Vector3.Zero;
		if (Input.IsPhysicalKeyPressed(Key.W)) { move -= Transform.Basis.Z; }
		if (Input.IsPhysicalKeyPressed(Key.S)) { move += Transform.Basis.Z; }
		if (Input.IsPhysicalKeyPressed(Key.A)) { move -= Transform.Basis.X; }
		if (Input.IsPhysicalKeyPressed(Key.D)) { move += Transform.Basis.X; }
		if (Input.IsPhysicalKeyPressed(Key.E)) { move += Vector3.Up; }
		if (Input.IsPhysicalKeyPressed(Key.Q)) { move -= Vector3.Up; }

		if (move.Length() > 0.0f)
		{
			float speed = Speed;
			if (Input.IsPhysicalKeyPressed(Key.Shift))
			{
				speed = FastSpeed;
			}
			Position += move.Normalized() * speed * (float)delta;
		}
	}

	public void SnapTo(Camera3D other)
	{
		GlobalTransform = other.GlobalTransform;
		Fov = other.Fov;
		KeepAspect = other.KeepAspect;

		Vector3 angles = GlobalTransform.Basis.GetEuler();
		_pitch = angles.X;
		_yaw = angles.Y;
		Rotation = new Vector3(_pitch, _yaw, 0.0f);
	}
}
```

- [ ] **Step 2: Add the fly camera and the scene furniture to `Main`**

In `scripts/Main.cs`, add the field `public FlyCamera Fly;` beside the others, and insert this before the `--self-test` block in `_Ready`:

```csharp
		BuildEnvironment();

		Fly = new FlyCamera();
		Fly.Name = "FlyCamera";
		AddChild(Fly);
		Fly.SnapTo(ChallengeCam);
		Fly.Current = true;
```

Then add these methods to `Main`:

```csharp
	// A floor grid and a marker at the challenge camera, so the geometry of
	// part (a) is visible rather than just asserted.
	public void BuildEnvironment()
	{
		ImmediateMesh grid = new ImmediateMesh();
		StandardMaterial3D lineMaterial = new StandardMaterial3D();
		lineMaterial.AlbedoColor = new Color(0.75f, 0.75f, 0.78f);
		lineMaterial.ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded;
		lineMaterial.VertexColorUseAsAlbedo = true;

		grid.SurfaceBegin(Mesh.PrimitiveType.Lines, lineMaterial);
		for (int i = -10; i <= 10; i++)
		{
			float at = i * 0.5f;
			grid.SurfaceSetColor(new Color(0.75f, 0.75f, 0.78f));
			grid.SurfaceAddVertex(new Vector3(at, 0.0f, -5.0f));
			grid.SurfaceAddVertex(new Vector3(at, 0.0f, 5.0f));
			grid.SurfaceAddVertex(new Vector3(-5.0f, 0.0f, at));
			grid.SurfaceAddVertex(new Vector3(5.0f, 0.0f, at));
		}
		grid.SurfaceEnd();

		MeshInstance3D floor = new MeshInstance3D();
		floor.Name = "FloorGrid";
		floor.Mesh = grid;
		AddChild(floor);

		BoxMesh box = new BoxMesh();
		box.Size = new Vector3(0.14f, 0.14f, 0.14f);
		StandardMaterial3D markerMaterial = new StandardMaterial3D();
		markerMaterial.AlbedoColor = new Color(0.95f, 0.65f, 0.10f);
		markerMaterial.ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded;

		_cameraMarker = new MeshInstance3D();
		_cameraMarker.Name = "CameraMarker";
		_cameraMarker.Mesh = box;
		_cameraMarker.MaterialOverride = markerMaterial;
		AddChild(_cameraMarker);
	}
```

and the field `private MeshInstance3D _cameraMarker;`. Extend `AimChallengeCamera` so the marker follows:

```csharp
	public void AimChallengeCamera(PoseFrame frame)
	{
		ChallengeCam.Position = frame.CameraGodot;
		ChallengeCam.LookAt(frame.CentroidGodot, Vector3.Up);
		if (_cameraMarker != null)
		{
			_cameraMarker.Position = frame.CameraGodot;
		}
	}
```

- [ ] **Step 3: Verify the self-test still passes**

```bash
dotnet build PoseProjection.csproj --nologo
timeout 180 godot-mono --headless --path . -- --self-test
echo "exit code: $?"
```

Expected: still `self-test: OK`, exit code 0. Adding a second camera must not disturb the projection — `ChallengeCam` is not `Current` any more, and `UnprojectPosition` does not require it to be.

If this now fails, `GodotProjector` is picking up the wrong camera. It holds a direct reference to `ChallengeCam`, so check that nothing reassigned it.

- [ ] **Step 4: Verify interactively**

```bash
timeout 60 godot-mono --path .
```

Fly with WASD, Q/E, Shift. Click to capture the mouse and look around; press Esc to release. Confirm the skeleton is a recognisable seated figure standing on the grid with an orange marker off to one side.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "Add the WASD fly camera, floor grid and challenge-camera marker"
```

---

### Task 9: The 2D projection overlay

**Files:**
- Create: `scripts/ProjectionOverlay.cs`
- Modify: `scripts/Main.cs`

**Interfaces:**
- Consumes: `IProjector`, `Framing`, `FramingTransform`, `ViewFraming`, `Bones`, `PoseFrame`.
- Produces: `partial class ProjectionOverlay : Control` with `Vector2[] Points`, `ViewFraming Mode`, `bool ShowPhotograph`, `Texture2D Photograph`, and `void Refresh(Vector2[] points)`; `Main` gains `public ProjectionOverlay Overlay`, `public bool TwoDimensionalView`, `public ProjectionMethod Method`, `public int FrameIndex`, `public void ShowFrame(int index)`, `public Vector2[] ProjectCurrent(ProjectionMethod method)`.

- [ ] **Step 1: Write `ProjectionOverlay`**

Create `scripts/ProjectionOverlay.cs`:

```csharp
using Godot;

// Draws the projected skeleton. In ImagePixels mode the coordinates are
// used exactly as computed, which is what makes the photograph overlay
// line up. FitToPose only rescales what is drawn.
public partial class ProjectionOverlay : Control
{
	private const float BoneWidth = 4.0f;
	private const float JointRadius = 6.0f;

	public Vector2[] Points;
	public ViewFraming Mode = ViewFraming.FitToPose;
	public bool ShowPhotograph;
	public Texture2D Photograph;

	public void Refresh(Vector2[] points)
	{
		Points = points;
		QueueRedraw();
	}

	public override void _Draw()
	{
		Vector2 size = new Vector2(Main.ImageSize, Main.ImageSize);
		if (ShowPhotograph && Photograph != null)
		{
			DrawTextureRect(Photograph, new Rect2(Vector2.Zero, size), false);
		}
		else
		{
			DrawRect(new Rect2(Vector2.Zero, size), Colors.White, true);
		}

		if (Points == null)
		{
			return;
		}

		ViewFraming mode = Mode;
		if (ShowPhotograph)
		{
			// Any rescaling would break registration with the photograph.
			mode = ViewFraming.ImagePixels;
		}
		FramingTransform t = Framing.Compute(Points, mode, Main.ImageSize, 0.1f);

		for (int b = 0; b < Bones.All.Length; b++)
		{
			Vector2 a = t.Apply(Points[Bones.All[b].A]);
			Vector2 c = t.Apply(Points[Bones.All[b].B]);
			DrawLine(a, c, Bones.ColorFor(Bones.All[b].Side), BoneWidth, true);
		}

		for (int j = 0; j < Points.Length; j++)
		{
			DrawCircle(t.Apply(Points[j]), JointRadius, new Color(0.10f, 0.10f, 0.10f));
		}
	}
}
```

- [ ] **Step 2: Add frame handling and mode switching to `Main`**

Add these fields to `Main`:

```csharp
	public ProjectionOverlay Overlay;
	public bool TwoDimensionalView;
	public ProjectionMethod Method = ProjectionMethod.ManualPinhole;
	public int FrameIndex;

	[Export] public ProjectionMethod DefaultMethod = ProjectionMethod.ManualPinhole;

	private CanvasLayer _canvas;
	private ManualProjector _manual = new ManualProjector();
	private GodotProjector _godot;
```

In `_Ready`, after the fly camera is created and before the `--self-test` block:

```csharp
		Method = DefaultMethod;
		_godot = new GodotProjector(ChallengeCam, Figure);

		_canvas = new CanvasLayer();
		_canvas.Name = "Canvas";
		AddChild(_canvas);

		Overlay = new ProjectionOverlay();
		Overlay.Name = "Overlay";
		Overlay.SetAnchorsPreset(Control.LayoutPreset.FullRect);
		Overlay.MouseFilter = Control.MouseFilterEnum.Ignore;
		Overlay.Visible = false;
		_canvas.AddChild(Overlay);
```

and replace the `Figure.ShowFrame(...)` / `AimChallengeCamera(...)` pair with `ShowFrame(0);`.

Add these methods:

```csharp
	public void ShowFrame(int index)
	{
		FrameIndex = Mathf.Clamp(index, 0, Data.Frames.Length - 1);
		PoseFrame frame = Data.Frames[FrameIndex];
		Figure.ShowFrame(frame);
		AimChallengeCamera(frame);

		if (Overlay != null)
		{
			Overlay.Photograph = LoadFrameTexture(FrameIndex);
			Overlay.Refresh(ProjectCurrent(Method));
		}
	}

	public Vector2[] ProjectCurrent(ProjectionMethod method)
	{
		PoseFrame frame = Data.Frames[FrameIndex];
		IProjector projector = _manual;
		if (method == ProjectionMethod.GodotUnproject)
		{
			projector = _godot;
		}
		projector.Begin(frame, Data.Focal);

		Vector2[] points = new Vector2[PoseData.JointCount];
		for (int j = 0; j < points.Length; j++)
		{
			points[j] = projector.Project(j);
		}
		return points;
	}

	public void SetTwoDimensionalView(bool on)
	{
		TwoDimensionalView = on;
		Overlay.Visible = on;
		ChallengeCam.Current = on;
		Fly.Current = !on;
		if (on)
		{
			Input.MouseMode = Input.MouseModeEnum.Visible;
		}
		Overlay.Refresh(ProjectCurrent(Method));
	}

	public static Texture2D LoadFrameTexture(int index)
	{
		string path = "res://data/frames/" + index.ToString("00") + ".png";
		return GD.Load<Texture2D>(path);
	}
```

- [ ] **Step 3: Build and re-run the self-test**

```bash
dotnet build PoseProjection.csproj --nologo
timeout 180 godot-mono --headless --path . -- --self-test
echo "exit code: $?"
```

Expected: `self-test: OK`, exit code 0.

- [ ] **Step 4: Add a temporary check that the overlay draws**

Temporarily add to the end of `_Ready` (after the self-test block):

```csharp
		SetTwoDimensionalView(true);
```

Run `timeout 60 godot-mono --path .` and confirm a large red/blue/black skeleton on white fills the window. Then remove the temporary line and rebuild.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "Draw the projected skeleton in a 2D overlay with both framings"
```

---

### Task 10: The control panel

**Files:**
- Create: `scripts/ControlPanel.cs`
- Modify: `scripts/Main.cs`

**Interfaces:**
- Consumes: `Main.ShowFrame`, `Main.SetTwoDimensionalView`, `Main.ProjectCurrent`, `Main.Overlay`, `Main.Fly`, `Main.ChallengeCam`, `CoordinateTable.MaxAbsDiff`.
- Produces: `partial class ControlPanel : PanelContainer` with `void Build(Main main)`, `void UpdateReadouts()` and `void SetStatus(string text)`.

- [ ] **Step 1: Write `ControlPanel`**

Create `scripts/ControlPanel.cs`:

```csharp
using Godot;

// Every operation other than moving the camera lives here. No shortcut
// keys: the keyboard belongs to the fly camera.
public partial class ControlPanel : PanelContainer
{
	private Main _main;
	private Label _frameLabel;
	private HSlider _frameSlider;
	private OptionButton _viewOption;
	private OptionButton _methodOption;
	private OptionButton _framingOption;
	private OptionButton _backgroundOption;
	private Label _agreementLabel;
	private Label _statusLabel;

	public void Build(Main main)
	{
		_main = main;

		SetAnchorsPreset(Control.LayoutPreset.TopLeft);
		Position = new Vector2(12.0f, 12.0f);
		CustomMinimumSize = new Vector2(280.0f, 0.0f);

		VBoxContainer box = new VBoxContainer();
		AddChild(box);

		_frameLabel = new Label();
		box.AddChild(_frameLabel);

		HBoxContainer stepper = new HBoxContainer();
		box.AddChild(stepper);

		Button previous = new Button();
		previous.Text = "<";
		previous.Pressed += OnPrevious;
		stepper.AddChild(previous);

		_frameSlider = new HSlider();
		_frameSlider.MinValue = 0;
		_frameSlider.MaxValue = _main.Data.Frames.Length - 1;
		_frameSlider.Step = 1;
		_frameSlider.CustomMinimumSize = new Vector2(180.0f, 0.0f);
		_frameSlider.ValueChanged += OnSliderChanged;
		stepper.AddChild(_frameSlider);

		Button next = new Button();
		next.Text = ">";
		next.Pressed += OnNext;
		stepper.AddChild(next);

		_viewOption = AddOption(box, "View", new string[] { "3D scene", "2D projection" });
		_viewOption.ItemSelected += OnViewSelected;

		_methodOption = AddOption(box, "Projection method", new string[] { "Manual pinhole", "Godot unproject" });
		_methodOption.Selected = (int)_main.Method;
		_methodOption.ItemSelected += OnMethodSelected;

		_framingOption = AddOption(box, "Framing", new string[] { "Fit to pose", "True image pixels" });
		_framingOption.ItemSelected += OnFramingSelected;

		_backgroundOption = AddOption(box, "Background", new string[] { "White", "Photograph" });
		_backgroundOption.ItemSelected += OnBackgroundSelected;

		Button snap = new Button();
		snap.Text = "Snap to challenge camera";
		snap.Pressed += OnSnap;
		box.AddChild(snap);

		Button export = new Button();
		export.Text = "Export tables and images";
		export.Pressed += OnExport;
		box.AddChild(export);

		_agreementLabel = new Label();
		box.AddChild(_agreementLabel);

		_statusLabel = new Label();
		_statusLabel.AutowrapMode = TextServer.AutowrapMode.WordSmart;
		_statusLabel.CustomMinimumSize = new Vector2(260.0f, 0.0f);
		box.AddChild(_statusLabel);

		Label help = new Label();
		help.Text = "WASD move, Q/E down/up, Shift faster,\nclick to look, Esc frees the cursor";
		box.AddChild(help);

		UpdateReadouts();
	}

	public void SetStatus(string text)
	{
		_statusLabel.Text = text;
	}

	public void UpdateReadouts()
	{
		_frameLabel.Text = "Frame " + (_main.FrameIndex + 1).ToString("00") + " / " + _main.Data.Frames.Length;
		_frameSlider.SetValueNoSignal(_main.FrameIndex);

		ProjectedFrame manual = new ProjectedFrame();
		manual.Frame = _main.FrameIndex;
		manual.Points = _main.ProjectCurrent(ProjectionMethod.ManualPinhole);

		ProjectedFrame godot = new ProjectedFrame();
		godot.Frame = _main.FrameIndex;
		godot.Points = _main.ProjectCurrent(ProjectionMethod.GodotUnproject);

		float worst = CoordinateTable.MaxAbsDiff(manual, godot);
		_agreementLabel.Text = "Methods agree to " + worst.ToString("0.0000") + " px";

		_framingOption.Disabled = _main.Overlay.ShowPhotograph;
	}

	private static OptionButton AddOption(VBoxContainer box, string caption, string[] items)
	{
		Label label = new Label();
		label.Text = caption;
		box.AddChild(label);

		OptionButton option = new OptionButton();
		for (int i = 0; i < items.Length; i++)
		{
			option.AddItem(items[i], i);
		}
		option.Selected = 0;
		box.AddChild(option);
		return option;
	}

	private void OnPrevious()
	{
		_main.ShowFrame(_main.FrameIndex - 1);
		UpdateReadouts();
	}

	private void OnNext()
	{
		_main.ShowFrame(_main.FrameIndex + 1);
		UpdateReadouts();
	}

	private void OnSliderChanged(double value)
	{
		_main.ShowFrame((int)value);
		UpdateReadouts();
	}

	private void OnViewSelected(long index)
	{
		_main.SetTwoDimensionalView(index == 1);
		UpdateReadouts();
	}

	private void OnMethodSelected(long index)
	{
		_main.Method = (ProjectionMethod)index;
		_main.Overlay.Refresh(_main.ProjectCurrent(_main.Method));
		UpdateReadouts();
	}

	private void OnFramingSelected(long index)
	{
		_main.Overlay.Mode = (ViewFraming)index;
		_main.Overlay.QueueRedraw();
	}

	private void OnBackgroundSelected(long index)
	{
		_main.Overlay.ShowPhotograph = index == 1;
		_main.Overlay.QueueRedraw();
		UpdateReadouts();
	}

	private void OnSnap()
	{
		_main.Fly.SnapTo(_main.ChallengeCam);
	}

	private async void OnExport()
	{
		SetStatus("exporting...");
		string message = await _main.RunExport();
		SetStatus(message);
	}
}
```

- [ ] **Step 2: Add the panel to `Main`**

Add the field `public ControlPanel Panel;` and, in `_Ready` after the overlay is created:

```csharp
		Panel = new ControlPanel();
		Panel.Name = "ControlPanel";
		_canvas.AddChild(Panel);
```

`Panel.Build(this)` must be called after `ShowFrame(0)`, because it reads `FrameIndex` and projects the current frame.

`Main._Ready` has been edited by Tasks 7, 8, 9 and 10. Its statement order must now be exactly this — several of the steps below depend on it:

```
1.  Data = PoseLoader.LoadFromResources();
2.  Figure  = new PoseFigure();  AddChild;  Figure.Build();
3.  ChallengeCam = new Camera3D();  configure Fov/KeepAspect/Near;  AddChild;
4.  BuildEnvironment();
5.  Fly = new FlyCamera();  AddChild;
6.  Method = DefaultMethod;  _godot = new GodotProjector(ChallengeCam, Figure);
7.  _canvas = new CanvasLayer();  AddChild;
8.  Overlay = new ProjectionOverlay();  configure;  _canvas.AddChild;
9.  Panel = new ControlPanel();  _canvas.AddChild;
10. ShowFrame(0);
11. Fly.SnapTo(ChallengeCam);  Fly.Current = true;
12. Panel.Build(this);
13. command-line argument loop (--self-test, --export-csv)
14. GD.Print(...)
```

Step 11 comes after step 10 because `SnapTo` copies the challenge camera's transform, and that transform is only aimed at the subject once `ShowFrame` has run. Step 12 comes after both, because `Build` projects the current frame to fill in the agreement label.

- [ ] **Step 3: Add a temporary export stub so this task builds on its own**

Add to `Main` (Task 11 replaces the body):

```csharp
	public async System.Threading.Tasks.Task<string> RunExport()
	{
		await ToSignal(GetTree(), SceneTree.SignalName.ProcessFrame);
		return "export not implemented yet";
	}
```

- [ ] **Step 4: Build and verify**

```bash
dotnet build PoseProjection.csproj --nologo
timeout 180 godot-mono --headless --path . -- --self-test   # expect OK, exit 0
timeout 90 godot-mono --path .
```

In the window: step frames with `<` and `>` and the slider; switch View to "2D projection" and back; switch Projection method and confirm the drawing does not visibly change; switch Background to "Photograph" and confirm the skeleton lands on the person and the Framing control greys out; click "Snap to challenge camera" in 3D view and confirm the viewpoint matches the 2D projection's. The agreement label should read `Methods agree to 0.0000 px` or similar.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "Add the control panel driving every operation from the UI"
```

---

### Task 11: Export the tables and images

**Files:**
- Create: `scripts/Exporter.cs`
- Modify: `scripts/Main.cs`

**Interfaces:**
- Consumes: `CoordinateTable`, `ProjectedFrame`, `Main.ShowFrame`, `Main.ProjectCurrent`, `Main.Overlay`, `Main.Panel`.
- Produces: `class Exporter` with constructor `Exporter(Main main)`, `void WriteTables()`, `async Task WriteImages()`, and `static string OutputDirectory()`.

- [ ] **Step 1: Write `Exporter`**

Create `scripts/Exporter.cs`:

```csharp
using System.IO;
using System.Threading.Tasks;
using Godot;

// Produces the challenge deliverables. Recorded coordinates are always true
// image pixels; only the white-background images are rescaled to be legible.
public class Exporter
{
	private Main _main;

	public Exporter(Main main)
	{
		_main = main;
	}

	public static string OutputDirectory()
	{
		string dir = ProjectSettings.GlobalizePath("res://out");
		Directory.CreateDirectory(dir);
		return dir;
	}

	public void WriteTables()
	{
		int count = _main.Data.Frames.Length;
		ProjectedFrame[] manual = new ProjectedFrame[count];
		ProjectedFrame[] godot = new ProjectedFrame[count];
		ProjectedFrame[] active = new ProjectedFrame[count];

		int restore = _main.FrameIndex;
		for (int f = 0; f < count; f++)
		{
			_main.ShowFrame(f);

			manual[f] = new ProjectedFrame();
			manual[f].Frame = f;
			manual[f].Points = _main.ProjectCurrent(ProjectionMethod.ManualPinhole);

			godot[f] = new ProjectedFrame();
			godot[f].Frame = f;
			godot[f].Points = _main.ProjectCurrent(ProjectionMethod.GodotUnproject);

			active[f] = new ProjectedFrame();
			active[f].Frame = f;
			active[f].Points = _main.ProjectCurrent(_main.Method);
		}
		_main.ShowFrame(restore);

		string dir = OutputDirectory();
		StreamWriter wide = new StreamWriter(Path.Combine(dir, "coords_wide.csv"));
		CoordinateTable.WriteWide(wide, active);
		wide.Dispose();

		StreamWriter longTable = new StreamWriter(Path.Combine(dir, "coords_long.csv"));
		CoordinateTable.WriteLong(longTable, manual, godot, _main.Data.JointNames);
		longTable.Dispose();

		StreamWriter agreement = new StreamWriter(Path.Combine(dir, "method_agreement.csv"));
		CoordinateTable.WriteAgreement(agreement, manual, godot);
		agreement.Dispose();
	}

	public async Task WriteImages()
	{
		string dir = OutputDirectory();

		bool wasTwoD = _main.TwoDimensionalView;
		ViewFraming wasMode = _main.Overlay.Mode;
		bool wasPhoto = _main.Overlay.ShowPhotograph;
		int restore = _main.FrameIndex;

		// The panel would otherwise be baked into every exported image.
		_main.Panel.Visible = false;
		_main.SetTwoDimensionalView(true);

		await Capture(dir, "proj_", ViewFraming.FitToPose, false);
		await Capture(dir, "overlay_", ViewFraming.ImagePixels, true);

		_main.Overlay.Mode = wasMode;
		_main.Overlay.ShowPhotograph = wasPhoto;
		_main.ShowFrame(restore);
		_main.SetTwoDimensionalView(wasTwoD);
		_main.Panel.Visible = true;
	}

	private async Task Capture(string dir, string prefix, ViewFraming mode, bool photograph)
	{
		_main.Overlay.Mode = mode;
		_main.Overlay.ShowPhotograph = photograph;

		for (int f = 0; f < _main.Data.Frames.Length; f++)
		{
			_main.ShowFrame(f);
			_main.Overlay.QueueRedraw();

			// Two frames, not one: the first lets the redraw be submitted,
			// the second guarantees it has been presented before the read.
			await _main.ToSignal(RenderingServer.Singleton, RenderingServer.SignalName.FramePostDraw);
			await _main.ToSignal(RenderingServer.Singleton, RenderingServer.SignalName.FramePostDraw);

			Image image = _main.GetViewport().GetTexture().GetImage();
			image.SavePng(Path.Combine(dir, prefix + f.ToString("00") + ".png"));
		}
	}
}
```

- [ ] **Step 2: Replace the export stub in `Main`**

```csharp
	public async System.Threading.Tasks.Task<string> RunExport()
	{
		Exporter exporter = new Exporter(this);
		exporter.WriteTables();
		await exporter.WriteImages();
		Panel.UpdateReadouts();
		return "wrote tables and 40 images to " + Exporter.OutputDirectory();
	}
```

- [ ] **Step 3: Add the headless CSV path**

Add a second branch inside the existing command-line loop (step 13 of the `_Ready` order in Task 10), directly below the `--self-test` branch:

```csharp
			if (args[i] == "--export-csv")
			{
				Exporter exporter = new Exporter(this);
				exporter.WriteTables();
				GD.Print("wrote tables to " + Exporter.OutputDirectory());
				GetTree().Quit(0);
				return;
			}
```

`WriteTables` calls `ShowFrame` and `ProjectCurrent`, both of which are ready by step 13. It never touches `Panel`, so it is safe headless.

- [ ] **Step 4: Verify the headless CSV export**

```bash
dotnet build PoseProjection.csproj --nologo
timeout 180 godot-mono --headless --path . -- --export-csv
wc -l out/coords_wide.csv out/coords_long.csv out/method_agreement.csv
head -1 out/coords_wide.csv | tr ',' '\n' | wc -l
cut -d, -f2 out/method_agreement.csv | tail -n +2 | sort -g | tail -1
```

Expected: `coords_wide.csv` 21 lines, `coords_long.csv` 561 lines, `method_agreement.csv` 21 lines; 29 header fields; the largest `max_abs_diff_px` under 0.01.

- [ ] **Step 5: Verify the image export**

```bash
timeout 180 godot-mono --path .
```

Click "Export tables and images", wait for the status line, then quit and check:

```bash
ls out/proj_*.png | wc -l      # expect 20
ls out/overlay_*.png | wc -l   # expect 20
```

Open `out/proj_00.png`: a large skeleton on white, no control panel visible. Open `out/overlay_00.png`: the skeleton sitting on the person in the photo, no control panel visible.

If the panel appears in the images, `Panel.Visible = false` is being set too late or reset too early. If an image shows the previous frame's pose, add a third `FramePostDraw` await in `Capture`. If every `overlay_*.png` has a white background instead of the photograph, the PNGs were never imported — run `godot-mono --headless --path . --quit-after 60` once and export again.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "Export the coordinate tables and the projection and overlay images"
```

---

### Task 12: The Python reference check and the README

**Files:**
- Create: `tools/reference_projection.py`, `tools/check_projection.sh`, `README.md`
- Modify: none

**Interfaces:**
- Consumes: `out/coords_long.csv` produced by `--export-csv`.
- Produces: `tools/check_projection.sh` exiting 0 when all three implementations agree.

- [ ] **Step 1: Write the Python reference**

Create `tools/reference_projection.py`:

```python
#!/usr/bin/env python3
"""Independent reference for the Hayes pose-projection challenge.

Recomputes every 2D coordinate from the raw data using nothing but the
standard library, then compares against the CSV the Godot tool exported.
Third opinion: if this and both C# implementations agree, the numbers are
almost certainly right.
"""

import csv
import math
import os
import sys

TOLERANCE = 0.01
JOINTS = 14
PRINCIPAL = 500.0


def load(data_dir):
    focal = float(open(os.path.join(data_dir, "focal.txt")).read().strip())
    rows = []
    for line in open(os.path.join(data_dir, "poses.txt")):
        line = line.strip()
        if line:
            rows.append([float(v) for v in line.split()])
    return focal, rows


def sub(a, b):
    return [a[0] - b[0], a[1] - b[1], a[2] - b[2]]


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return [a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]]


def unit(a):
    n = math.sqrt(dot(a, a))
    return [a[0] / n, a[1] / n, a[2] / n]


def project_frame(row, focal):
    camera = row[0:3]
    joints = [row[3 + 3 * i:6 + 3 * i] for i in range(JOINTS)]

    centroid = [sum(j[k] for j in joints) / JOINTS for k in range(3)]

    forward = unit(sub(centroid, camera))
    right = unit(cross(forward, [0.0, 0.0, 1.0]))
    up = cross(right, forward)

    points = []
    for joint in joints:
        d = sub(joint, camera)
        z = dot(d, forward)
        u = focal * dot(d, right) / z + PRINCIPAL
        v = PRINCIPAL - focal * dot(d, up) / z
        points.append((u, v))
    return points


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    focal, rows = load(os.path.join(root, "data"))

    expected = {}
    for frame, row in enumerate(rows):
        for joint, (u, v) in enumerate(project_frame(row, focal)):
            expected[(frame, joint)] = (u, v)

    path = os.path.join(root, "out", "coords_long.csv")
    if not os.path.exists(path):
        print("missing %s; run: godot-mono --headless --path . -- --export-csv" % path)
        return 1

    checked = 0
    worst = 0.0
    failures = []
    with open(path) as handle:
        for record in csv.DictReader(handle):
            key = (int(record["frame"]), int(record["joint_id"]))
            want_u, want_v = expected[key]
            du = abs(float(record["u"]) - want_u)
            dv = abs(float(record["v"]) - want_v)
            worst = max(worst, du, dv)
            checked += 1
            if du > TOLERANCE or dv > TOLERANCE:
                failures.append("frame %d joint %d method %s: du=%.4f dv=%.4f"
                                % (key[0], key[1], record["method"], du, dv))

    if checked != 2 * len(rows) * JOINTS:
        print("expected %d rows, checked %d" % (2 * len(rows) * JOINTS, checked))
        return 1

    for line in failures:
        print(line)
    if failures:
        print("FAIL: %d of %d rows disagree" % (len(failures), checked))
        return 1

    print("OK: %d rows match the reference, worst deviation %.6f px" % (checked, worst))
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 2: Write the check script**

Create `tools/check_projection.sh`:

```bash
#!/usr/bin/env bash
# Runs every check: unit tests, the in-engine self-test, and the Python
# reference comparison.
set -e

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

echo "== unit tests =="
(cd tests/PoseProjection.Tests && dotnet test --nologo)

echo "== build =="
dotnet build PoseProjection.csproj --nologo

echo "== in-engine self-test =="
godot-mono --headless --path . -- --self-test

echo "== export tables =="
godot-mono --headless --path . -- --export-csv

echo "== python reference =="
python3 tools/reference_projection.py

echo "all checks passed"
```

Then: `chmod +x tools/check_projection.sh`

- [ ] **Step 3: Run the full check**

Run: `./tools/check_projection.sh`
Expected: unit tests pass, `self-test: OK`, and `OK: 560 rows match the reference, worst deviation ...` with the deviation below 0.01. Final line `all checks passed`.

- [ ] **Step 4: Write the README**

Create `README.md`:

```markdown
# Hayes Pose Projection

An answer to the challenge on the
[Human Pose Estimation via AI/ML](https://ics.uci.edu/~wayne/research/students/)
project, built as an interactive Godot 4 tool in C#.

## Running it

Requires the .NET build of Godot 4.7 (`godot-mono`) and the .NET SDK.

    dotnet build PoseProjection.csproj
    godot-mono --path .

Move the camera with `W A S D`, `Q`/`E` for down and up, `Shift` to go faster.
Click in the window to look around with the mouse; `Esc` frees the cursor.
Everything else is in the control panel: stepping through the 20 frames,
switching between the 3D scene and the 2D projection, choosing the projection
method, the framing and the background, snapping the free camera to the
challenge camera, and exporting.

## The answers

**(a) Camera orientation.** For each frame the camera sits at the position
given in the first three columns of `poses.txt` and looks at the centroid of
that frame's 14 joints, with no roll. The centroid is used rather than the hip
so that crouched and reaching poses stay framed.

**(b) The 2D projection.** A pinhole model with focal length 1148.6 and the
principal point at the centre of the 1000x1000 frame. `focal.txt` is labelled
millimetres, but the value is in pixels: this is Human3.6M, whose cameras have
f of about 1145 px, and no sensor size is supplied that would let a millimetre
figure be used. The resulting field of view, 47.048 degrees, matches the frames.

**(c) The table.** `out/coords_wide.csv` holds all 280 coordinates, one row per
frame and 28 columns of `u0,v0 ... u13,v13`, mirroring the layout of
`poses.txt`. `out/coords_long.csv` is the same data one row per joint, and
includes both projection methods.

**(d) Superimposed on the frames.** `out/overlay_00.png` through
`overlay_19.png`. As the challenge notes, the reconstructed viewpoint is not
identical to the photograph's, so the fit is close but not exact.

## Two projection methods

The projection is implemented twice and either can be selected in the control
panel:

- **Manual pinhole** builds the camera basis by hand from the raw millimetre
  data and applies `u = f * x / z + 500` directly. It never touches the scene
  graph.
- **Godot unproject** places a `Camera3D` at the same position with a field of
  view derived from the focal length and calls `unproject_position`.

They agree to within 0.0001 px, which is not a coincidence: Godot's perspective
projection over a viewport of height H with vertical FOV theta gives
`v = H/2 * (1 - (y/z) / tan(theta/2))`, and with `tan(theta/2) = 500/f` and
H = 1000 that is exactly `500 - f * y / z`. The panel shows the live difference
between them, and `out/method_agreement.csv` records it per frame.

The viewport is locked to 1000x1000 in `project.godot`. This matters: without
it, `unproject_position` would return coordinates that depend on the window
size, and the exported table would not be verifiable against the frames.

## Checks

    ./tools/check_projection.sh

Runs the unit tests, an in-engine headless comparison of the two projectors
across all 280 points, and `tools/reference_projection.py`, a third
implementation in plain Python that recomputes every coordinate from the raw
data and diffs it against the exported table.

## Layout

    data/         the contents of Pose.zip
    scripts/      the C# source
    scenes/       the Godot scene
    tests/        xUnit tests for the engine-free classes
    tools/        the Python reference and the check script
    out/          exported tables and images (not committed)
```

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "Add the Python reference check, the check script and the README"
```

---

## Final verification

- [ ] Run `./tools/check_projection.sh` — every stage passes.
- [ ] Run `godot-mono --path .` and exercise every control in the panel.
- [ ] Confirm `out/proj_00.png` shows a large legible skeleton on white with no UI.
- [ ] Confirm `out/overlay_00.png` puts the skeleton on the seated person.
- [ ] Confirm `git status` is clean and `out/` is not tracked.
