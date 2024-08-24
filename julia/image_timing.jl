using CSV, DataFrames, Plots, LaTeXStrings

function load_data()
    data_path = "./"*dirname(relpath(@__FILE__))*"/"
    data = CSV.File(data_path*"../csv/image_timing_dim.csv") |> DataFrame
    count = Vector(data[:, 1])
    t = Vector(data[:, 2])
    plt_dim = plot(count, t, dpi=300, label="seq", xlabel="width/height", ylabel=L"$\mu s$", title="Time to render image w.r.t the number of pixels")
    t = Vector(data[:, 3])
    plot!(count, t, label="simd inner")
    t = Vector(data[:, 4])
    plot!(count, t, label="simd pixels")

    data = CSV.File(data_path*"../csv/image_timing_gauss_64px.csv") |> DataFrame
    count = Vector(data[:, 1].^2)
    t = Vector(data[:, 2])
    plt_64px = plot(count, t, dpi=300, label="simd inner", xlabel="count", ylabel=L"$\mu s$", title="Time to render image (64px x 64px)\nw.r.t the number of gaussians")
    t = Vector(data[:, 3])
    plot!(count, t, label="simd pixels")

    data = CSV.File(data_path*"../csv/image_timing_gauss_128px.csv") |> DataFrame
    count = Vector(data[:, 1].^2)
    t = Vector(data[:, 2])
    plt_128px = plot(count, t, dpi=300, label="simd inner", xlabel="count", ylabel=L"$\mu s$", title="Time to render image (128px x 128px)\nw.r.t the number of gaussians")
    t = Vector(data[:, 3])
    plot!(count, t, label="simd pixels")

    data = CSV.File(data_path*"../csv/image_timing_gauss_256px.csv") |> DataFrame
    count = Vector(data[:, 1].^2)
    t = Vector(data[:, 2])
    plt_256px = plot(count, t, dpi=300, label="simd inner", xlabel="count", ylabel=L"$\mu s$", title="Time to render image (256px x 256px)\nw.r.t the number of gaussians")
    t = Vector(data[:, 3])
    plot!(count, t, label="simd pixels")

    data = CSV.File(data_path*"../csv/image_timing_gauss_512px.csv") |> DataFrame
    count = Vector(data[:, 1].^2)
    t = Vector(data[:, 2])
    plt_512px = plot(count, t, dpi=300, label="simd inner", xlabel="count", ylabel=L"$\mu s$", title="Time to render image (512px x 512px)\nw.r.t the number of gaussians")
    t = Vector(data[:, 3])
    plot!(count, t, label="simd pixels")

    return plt_dim, plt_64px, plt_128px, plt_256px, plt_512px
end

if abspath(PROGRAM_FILE) == @__FILE__
    ext = ".tex"
    img_dir = "./"*dirname(relpath(@__FILE__))*"/../thesis/plots"
    if "--as-image" in ARGS || "-i" in ARGS
        ext = ".png"
        img_dir = "./"*dirname(relpath(@__FILE__))*"/../images"
    else
        pgfplotsx()
    end
    if !isdir(img_dir)
        mkdir(img_dir)
    end
    plt_dim, plt_64px, plt_128px, plt_256px, plt_512px = load_data()
    savefig(plt_dim, img_dir*"/"*ENV["ARCH"]*"_image_timing_dim"*ext)
    savefig(plt_64px, img_dir*"/"*ENV["ARCH"]*"_image_timing_64px"*ext)
    savefig(plt_128px, img_dir*"/"*ENV["ARCH"]*"_image_timing_128px"*ext)
    savefig(plt_256px, img_dir*"/"*ENV["ARCH"]*"_image_timing_256px"*ext)
    savefig(plt_512px, img_dir*"/"*ENV["ARCH"]*"_image_timing_512px"*ext)
end
