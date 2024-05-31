using CSV, DataFrames, Plots, LaTeXStrings

function load_data()
    data_path = "./"*dirname(relpath(@__FILE__))*"/"
    data = CSV.File(data_path*"../csv/image_timing.csv") |> DataFrame
    count = Vector(data[:, 1])
    t = Vector(data[:, 2])
    plt = plot(count, t, dpi=300, label="seq", xlabel="width/height", ylabel=L"$\mu s$", title="Time to render image w.r.t the number of pixels")
    t = Vector(data[:, 3])
    plot!(count, t, label="simd inner")
    t = Vector(data[:, 4])
    plot!(count, t, label="simd pixels")
    return plt
end

if abspath(PROGRAM_FILE) == @__FILE__
    img_dir = "./"*dirname(relpath(@__FILE__))*"/../images"
    if !isdir(img_dir)
        mkdir(img_dir)
    end
    plt = load_data()
    savefig(plt, img_dir*"/image_timing.png")
end
