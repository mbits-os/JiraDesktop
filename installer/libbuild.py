import subprocess, tempfile, os

__all__ = ["Next", "call", "call_simple", "call_direct", "call_", "Version", "JiraVersion", "TaggedVersion", "Tag", "Branch", "Steps"]

Next = True

def __present(out, args):
	out.write("$ %s\n" % " ".join(args))
	out.flush()

def call(out, *args):
	__present(out, args)
	ret = subprocess.call(args, stdout=out, stderr=out)
	if ret: exit(ret)
	out.flush()

def call_simple(out, *args):
	__present(out, args)
	try: out.write(subprocess.check_output(args, stderr=subprocess.STDOUT))
	except subprocess.CalledProcessError as e:
		if e.output is not None:
			out.write(e.output)
		print(out.content)
		print(e)
		exit(e.returncode)

def call_direct(out, *args):
	__present(out, args)
	ret = subprocess.call(args)
	if ret: exit(ret)

def call_(out, *args):
	return subprocess.call(args)

def Version(next = None):
	args = [ "python", "version.py", "--in", "../apps/Tasks/src/version.h", "!SEMANTIC"]
	if next is not None: args += ["--next", "PROGRAM_VERSION_BUILD"]
	return subprocess.check_output(args).strip()

def JiraVersion():
	return subprocess.check_output([ "python", "version.py", "--in", "../apps/Tasks/src/version.h",
		"{PROGRAM_VERSION_MAJOR}.{PROGRAM_VERSION_MINOR}"]).strip()

def TaggedVersion(tag):
	tmp = tempfile.NamedTemporaryFile(prefix='version.h.', dir='.', delete=False)
	ret = subprocess.call([ "git", "show", tag + ":apps/Tasks/src/version.h" ], stdout=tmp)
	if ret:
		exit(ret)
	fname = tmp.name
	tmp.close()
	ret = subprocess.check_output([ "python", "version.py", "--in", fname, "!SEMANTIC"]).strip()
	os.remove(fname)
	return ret

def Tag():
	return subprocess.check_output([ "python", "version.py", "--in", "../apps/Tasks/src/version.h", "!NIGHTLY"]).strip()

def Branch():
	try:
		var = subprocess.check_output([ "git", "rev-parse", "--abbrev-ref", "HEAD"]).strip()
		return var
	except subprocess.CalledProcessError as e: return ""

class Step:
	def __init__(self, title, callable, *args):
		self.title = title
		self.callable = callable
		self.args = args
		self.visible = True
	def __call__(self, out, pos, max):
		print("Step %s/%s %s" % (pos, max, self.title))
		print("\n[%s]" % self.title, file=out)
		self.callable(out, *self.args)

class HiddenStep(Step):
	def __init__(self, callable, *args):
		Step.__init__(self, None, callable, *args)
		self.visible = False
	def __call__(self, out, pos, max):
		self.callable(out, *self.args)

class Steps:
	def __init__(self):
		self.steps = []
		self.count = 0

	def step(self, title, callable, *args):
		self.steps.append(Step(title, callable, *args))
		self.count += 1
		return self

	def hidden(self, callable, *args):
		self.steps.append(HiddenStep(callable, *args))
		return self

	def run(self, out, dry):
		i = 1
		max = self.count

		if dry:
			for step in self.steps:
				if not step.visible: continue

				if len(step.args):
					print("%s/%s" % (i, max), step.title, "(%s)" % " ".join([str(arg) for arg in step.args]))
				else:
					print("%s/%s" % (i, max), step.title)
				i += 1
		else:
			for step in self.steps:
				step(out, i, max)
				i += 1
