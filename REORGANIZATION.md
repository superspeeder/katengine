# What's happening on this branch?

This branch is going to be a reorganization (and largely a rewrite) of existing code. There might be a couple bits and pieces that I copy over which I have no reason to change, but for the most part things need to change and will.

# What's the final version going to look like?

There will be a lot more code seperation. Global state won't be a single struct but likely a bunch of extern variables and other things put into a global state namespace. Code will be better seperated into namespaces.
There will (hopefully) be better and more documentation, and (even more hopefully) there will be some level of automated tests (still need to learn how to get those working for c++).
There will be some new tooling for making development easier (ability to load some stuff from config files, or tools to generate code based on those files).

## A better hierarchy

Here is a new map of what the organization will be (only the namespaces for now, this also includes the organization of a bunch of systems which havent been written yet, but are planned):
(this is 100% me just pre planning feature creep but since I want to learn & mess with these things I might as well write them down)

> Worth noting that some of these may go away if I decide I don't need to implement them, or may change as I reorganize again. Also worth noting that some of these are likely going to just be wrappers around another library, as not all of this is worth implementing myself.

```
kat

kat::os
kat::os::window
kat::os::sys
kat::os::opt

kat::utils
kat::utils::threading

kat::render
kat::render::resource
kat::render::resource::pipeline
kat::render::resource::shader
kat::render::resource::mesh
kat::render::resource::optimizer
kat::render::graph
kat::render::graph::utils
kat::render::graph::optimizer
kat::render::raytracing
kat::render::abstraction
kat::render::opt

kat::vk
kat::vk::utils

kat::memtools
kat::opt
kat::config

kat::resource
kat::resource::loader
kat::resource::image
kat::resource::mesh
kat::resource::tilemap
kat::resource::config
kat::resource::database

kat::scripting
kat::scripting::bindings

kat::application
kat::debug
kat::log

kat::procedural
kat::procedural::noise
kat::procedural::noise::simplex
kat::procedural::noise::cell
kat::procedural::wfc

kat::algorithm
kat::algorithm::sort
kat::algorithm::packing
kat::algorithm::delauney
kat::algorithm::voronoi
kat::algorithm::minimax
kat::algorithm::pathfinding
kat::algorithm::pathfinding::astar
kat::algorithm::pathfinding::dijkstras
kat::algorithm::collisions
kat::algorithm::convexhull
kat::algorithm::minimumbb
kat::algorithm::nearestneighbor
kat::algorithm::marchingtriangles

kat::data
kat::data::quadtree
kat::data::octree
kat::data::kdtree

kat::math
kat::math::camera
kat::math::transform
kat::math::opt
kat::math::linalg
kat::math::geometry

kat::cellular_automata
kat::compute

kat::network
kat::network::http
kat::network::http::client
kat::network::http::server
kat::network::tcp
kat::network::tcp::client
kat::network::tcp::server
kat::network::udp
kat::network::udp::client
kat::network::udp::server
kat::network::encryption
kat::network::encryption::tls
kat::network::encryption::jwt

kat::cache
kat::database

kat::physics
kat::physics::p2d;
kat::physics::p3d;

kat::ai
kat::ai::statemachine

kat::scene
kat::scene::netsync
kat::scene::nodegraph

kat::ecs

kat::ui
```


## Various other things that I haven't yet placed in the hierarchy but I plan to implement
* text rendering
* asynchronous tasking
* input systems
* (a bunch more stuff but I'm done writing this for now and want to write code)

