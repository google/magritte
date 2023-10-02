#
# Copyright 2021-2022 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Provides BUILD macros for Magritte graphs.

magritte_graph creates a mediapipe_simple_subgraph along with
a dependencies cc_library and an exported graph file.
"""

load("@mediapipe//mediapipe/framework/tool:mediapipe_graph.bzl", "mediapipe_simple_subgraph")
load("@bazel_skylib//lib:paths.bzl", "paths")

# String constant to be used as a tag value to mark targets whose data
# dependencies should be included when the magritte_runtime_data (defined
# below) is used to collect them.
# This tag is added automatically to graphs defined via the magritte_graph
# macro below. Add it manually to other cc_library targets with runtime data
# dependencies such as calculators.
magritte_data_deps_tag = "magritte-target-with-data-deps"

def magritte_graph(
        name,
        register_as,
        graph,
        deps = [],
        data = [],
        tags = [],
        visibility = None,
        testonly = None,
        **kwargs):
    """Defines and registers a magritte graph by creating various targets, see below.

    This macro creates the following targets:
      * {name}: a mediapipe_simple_subgraph, registered as given in registered_as (effectively
        a cc_library)
      * {graph}: exported graph file (only if visibility is not private)
      * {name}_graph: a mediapipe binary graph, with the graph file name being {name}.binarypb

    Args:
      name: name of the graph target to define.
      register_as: name used to invoke this graph in supergraphs. Should be in
          CamelCase.
      graph: the BUILD label of a text-format Drishti graph.
      deps: any calculators or subgraphs directly used by this graph.
      data: any data dependencies required by the graph (e.g., tflite models).
      visibility: The list of packages the graph should be visible to.
      testonly: pass 1 if the graph is to be used only for tests.
      tags: tags to be added to the targets.
      **kwargs: Remaining keyword args, forwarded to mediapipe_simple_subgraph
          and generated test targets.
      """
    if visibility != ["//visibility:private"] and visibility != [":__subpackages__"]:
        native.exports_files([graph])
    mediapipe_simple_subgraph(
        name = name,
        register_as = register_as,
        graph = graph,
        deps = deps,
        data = data,
        tags = tags + [magritte_data_deps_tag],
        visibility = visibility,
        testonly = testonly,
        **kwargs
    )

# Provider to capture information about Magritte graphs and other targets
# marked with the magritte_data_deps_tag, required for data dependencies
# rule below.
MagritteDataInfo = provider(
    doc = "Magritte data information",
    fields = {
        "data_dependencies": "a target's runtime data dependencies",
    },
)

# Aspect that extends all dependencies of a target marked with the
# magritte_data_deps_tag, collecting their data dependencies.
def _data_deps_aspect_impl(_, ctx):
    # Skip all dependencies that do not have the magritte_data_deps_tag.
    if not (hasattr(ctx.rule.attr, "tags") and magritte_data_deps_tag in ctx.rule.attr.tags):
        return []
    transitive = []
    for target in ctx.rule.attr.data:
        transitive.append(target.files)
    for dep in getattr(ctx.rule.attr, "deps", []):
        if MagritteDataInfo in dep:
            transitive.append(dep[MagritteDataInfo].data_dependencies)
    data_depset = depset(
        direct = [],
        transitive = transitive,
    )
    return [MagritteDataInfo(data_dependencies = data_depset)]

_data_deps_aspect = aspect(
    implementation = _data_deps_aspect_impl,
    doc = "An aspect that can be applied to targets marked with the " +
          "magritte_data_deps_tag (thus including Magritte graphs, more " +
          "precisely, the underlying cc_library target with the same name " +
          "defined by the magritte_graph macro via the " +
          "mediapipe_simple_subgraph macro) and collects all data " +
          "dependencies. It does so by extending the deps attribute of the " +
          "target.",
    attr_aspects = ["deps"],
)

# Rule for Magritte runtime data dependencies target. Combines all data
# dependencies found by the aspect above.
def _magritte_runtime_data_impl(ctx):
    return [DefaultInfo(files = depset(
        direct = [],
        transitive = [dep[MagritteDataInfo].data_dependencies for dep in ctx.attr.deps],
    ))]

magritte_runtime_data = rule(
    implementation = _magritte_runtime_data_impl,
    doc = """A rule to create data dependencies for Magritte targets (e.g.,
          graphs or calculators). Targets defined with this rule can be
          referenced e.g. in the `assets` attribute of an `android_binary`
          target to make the data dependencies of a graph available to the
          binary.""",
    attrs = {
        "deps": attr.label_list(
            doc = """List of Magritte targets whose data dependencies to
                  expose. Targets listed here must either have been defined
                  with the magritte_graph macro from this file, or have a
                  magritte_data_deps_tag tag.""",
            mandatory = True,
            allow_empty = False,
            providers = [MagritteDataInfo],
            aspects = [_data_deps_aspect],
        ),
    },
)

# The next two functions are inspired from Bazel Skylib's copy_file, see
# https://github.com/bazelbuild/bazel-skylib/blob/main/rules/private/copy_file_private.bzl.

# Copies a file to another file. If the destination directory does not exist
# it will be created. (Bash version)
def _copy_action_bash(ctx, input_file, output_file):
    ctx.actions.run_shell(
        outputs = [output_file],
        inputs = [input_file],
        arguments = [input_file.path, output_file.dirname],
        command = "mkdir -p \"$2\" && cp -f \"$1\" \"$2\"",
        use_default_shell_env = True,
    )

# Copies a file to another file. If the destination directory does not exist
# it will be created. (Windows version)
def _copy_action_windows(ctx, input_file, output_file):
    bat = ctx.actions.declare_file("%s-%s-cmd.bat" % (ctx.label.name, hash(src.path)))
    file_to_copy = input_file.path.replace("/", "\\")
    destination_folder = output_file.dirname.replace("/", "\\")
    ctx.actions.write(
        output = bat,
        content = "@mkdir \"%s\"\n@copy /Y \"%s\" \"%s\"" %
                  (destination_folder, file_to_copy, destination_folder),
        is_executable = True,
    )
    ctx.actions.run(
        inputs = [input_file],
        tools = [bat],
        outputs = [output_file],
        executable = "cmd.exe",
        arguments = ["/C", bat.path.replace("/", "\\")],
        use_default_shell_env = True,
    )

# Implementation function for rule below.
def _magritte_resources_folder_impl(ctx):
    output_folder = ctx.attr.name
    created_files = []
    for file in ctx.files.runtime_data:
        relative_path = file.path.removeprefix("external/mediapipe/")
        output_file = ctx.actions.declare_file(paths.join(output_folder, relative_path))
        created_files.append(output_file)
        if ctx.attr.is_windows:
            _copy_action_windows(ctx, file, output_file)
        else:
            _copy_action_bash(ctx, file, output_file)
    return [DefaultInfo(files = depset(direct = created_files))]

# Private rule to create a resources folder, called by the macro below (see
# the documentation of that one). This rule is private because we want to hide
# the Windows/non-Windows case distinction from the user. The macro below
# calls this rule with the correct value for the is_windows parameter.
_magritte_resources_folder = rule(
    implementation = _magritte_resources_folder_impl,
    attrs = {
        "runtime_data": attr.label(
            mandatory = True,
        ),
        "is_windows": attr.bool(mandatory = True),
    },
)

def magritte_resources_folder(name, runtime_data, **kwargs):
    """Creates a folder that can be used as a MediaPipe resource folder.

    The output folder will have the same name as the target and should be
    given as value of the resource_root_dir command line flag to binaries on
    desktop environments.

    Args:
      name: Target name.
      runtime_data: A magritte_runtime_data target whose runtime data files
        should be copied into the folder.
      **kwargs: Remaining keyword args, forwarded to rule implementation.
    """
    _magritte_resources_folder(
        name = name,
        runtime_data = runtime_data,
        is_windows = select({
            "@bazel_tools//src/conditions:host_windows": True,
            "//conditions:default": False,
        }),
        **kwargs
    )

def magritte_graph_binary_filename_from_target(label):
    """Transforms a magritte_graph BUILD label to the filename of the corresponding binary graph.

    This will be the name of the target (without path) followed by ".binarypb".

    Args:
      label: the BUILD label of a magritte_graph.
    Returns:
      The .binarypb filename for the corresponding binary graph.
    """

    name = label.split(":")[-1] if ":" in label else label.split("/")[-1]
    return name + ".binarypb"
