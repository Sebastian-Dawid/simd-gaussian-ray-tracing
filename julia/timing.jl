using CSV, DataFrames, Plots, LaTeXStrings

function load_data()
    data_path = "./"*dirname(relpath(@__FILE__))*"/"
    data = CSV.File(data_path*"../csv/timing.csv") |> DataFrame
    count = Vector(data[:, 1])
    t = Vector(data[:, 2])
    plot(count, t, dpi=300, label="spline", xlabel="count", ylabel=L"$\mu s$", title="Time per pixel w.r.t. the number of gaussians\nin the scene")
    t = Vector(data[:, 3])
    plot!(count, t, label="mirror")
    t = Vector(data[:, 4])
    plot!(count, t, label="taylor")
    t = Vector(data[:, 5])
    plot!(count, t, label="std::erf")
end

if abspath(PROGRAM_FILE) == @__FILE__
    img_dir = "./"*dirname(relpath(@__FILE__))*"/../images"
    if !isdir(img_dir)
        mkdir(img_dir)
    end
    savefig(load_data(), img_dir*"/timing.png")
end
