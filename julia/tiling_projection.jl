using LinearAlgebra, Plots, ForwardDiff

camera_rot(θ) = [cos(θ) 0 -sin(θ); 0 1 0; sin(θ) 0 cos(θ)]
rot(u, θ) = [
(cos(θ) + u[3]^2 * (1 - cos(θ)))             (u[1] * u[2] * (1 - cos(θ)) - u[3] * sin(θ)) (u[1] * u[3] * (1 - cos(θ)) + u[2] * sin(θ));
(u[1] * u[2] * (1 - cos(θ) + u[3] * sin(θ))) (cos(θ) + u[2]^2 * (1 - cos(θ)))             (u[2] * u[3] * (1 - cos(θ)) - u[1] * sin(θ));
(u[1] * u[3] * (1 - cos(θ) - u[2] * sin(θ))) (u[2] * u[3] * (1 - cos(θ)) + u[1] * sin(θ)) (cos(θ) + u[3]^2 * (1 - cos(θ)))
]

# adapted from: https://github.com/graphdeco-inria/diff-gaussian-rasterization/blob/59f5f77e3ddbac3ed9db93ec2cfe99ed6c5d121d/cuda_rasterizer/forward.cu
jacobian(x, focal=[1;1]) = [
focal[1]/x[3] 0 -(focal[1] * x[1])/x[3]^2;
0 focal[2]/x[3] -(focal[2] * x[2])/x[3]^2;
0 0 0
]

function project_gauss(cam_angle, cam_pos, mu, V)
    W = camera_rot(cam_angle)
    V_cam = W * V * W'
    # J = ForwardDiff.jacobian(x -> [x[1]/x[3]; x[2]/x[3]; norm(x)], mu)
    J = jacobian([mu[1]/mu[3]; mu[2]/mu[3]; norm(mu)])
    sigma = J[1:2, 1:2] * V_cam[1:2, 1:2] * J'[1:2, 1:2]
    proj = W * mu + cam_pos
    p = (proj / proj[3])[1:2]
    plt = heatmap(-1:.05:1, -1:.05:1, (x, y) -> exp( -.5 * transpose([x; y] - p)*inv(sigma)*([x; y] - p)))
    return plt
end

# use the largest scaling factor of the covariance matrix as the sigma here
function gaussian_in_tile(tw, th, mu, sigma)
    centers = permutedims(hcat([[ x + tw/2; y + th/2 ] for x in -1:tw:(1-tw) for y in -1:th:(1-th)]...))
    dists = map(abs, centers' .- mu)
    return map(x -> ((x[1] >= tw/2 + 2 * sigma) + (x[2] >= th/2 + 2 * sigma)) == 0, eachcol(dists))
end
