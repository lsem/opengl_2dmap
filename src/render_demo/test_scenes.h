#pragma once

void generate_random_roads() {
    DebugCtx dctx;

    //
    // Generate random polylines
    //
    const double k = 100.0;

    vector<p32> all_roads_triangles(100000); // todo: calculate size correctly
    vector<p32> all_roads_aa_triangles(100000);
    size_t current_offset = 0;
    size_t current_aa_offset = 0;

    srand(clock());

    for (int i = 0; i < 200; ++i) {
        int random_segments_count = rand() % 30 + 4;
        double random_vector_angle = ((rand() % 360) / 360.0) * 2 * M_PI;
        const int scater = 60000;
        v2 origin = v2{MASTER_ORIGIN_X, MASTER_ORIGIN_Y} +
                    (v2{rand() % scater - (scater / 2), rand() % scater - (scater / 2)});
        v2 prev_p = origin;

        vector<p32> random_polyline = {from_v2(prev_p)};
        for (int s = 0; s < random_segments_count; s++) {
            double random_length = (double)(rand() % 10000 + 10000);
            double random_angle = random_vector_angle + ((rand() % 60 + 30) / 180.0) * M_PI;
            auto random_direction = normalized(v2(std::cos(random_angle), std::sin(random_angle)));
            // log_debug("random_length: {}", random_length);
            // log_debug("random_angle: {} ({})", random_angle,
            //(random_angle / M_PI) * 180.0);

            v2 p = prev_p + random_direction * random_length;

            if (p.x < 0.0 || p.y < 0.0) {
                log_warn("out of canvas. truncate road");
                break;
            }
            // dctx.add_line(prev_p, p, colors::grey);
            prev_p = p;

            random_polyline.push_back(from_v2(p));
        }
        if (random_polyline.size() < 3) {
            log_warn("two small road, skip");
            continue;
        }

        const size_t one_road_triangles_vertex_count = (random_polyline.size() - 1) * 12;
        span<p32> random_road_triangles_span(all_roads_triangles.data() + current_offset,
                                             one_road_triangles_vertex_count);

        current_offset += one_road_triangles_vertex_count;

        vector<p32> outline_output(random_polyline.size() *
                                   2); // outline must be have two lines per one segment.

        roads::tesselation::generate_geometry<roads::tesselation::FirstPassSettings>(
            random_polyline, random_road_triangles_span, outline_output, 2000.0, dctx);

        const size_t one_road_aa_triangles_vertex_count = (outline_output.size() - 1) * 12;
        span<p32> random_road_aa_triangles_span(all_roads_aa_triangles.data() + current_aa_offset,
                                                one_road_aa_triangles_vertex_count);
        current_aa_offset += one_road_aa_triangles_vertex_count;

        vector<p32> no_outline_for_aa_pass; // just fake placeholder.
        roads::tesselation::generate_geometry<roads::tesselation::AAPassSettings>(
            outline_output, random_road_aa_triangles_span, no_outline_for_aa_pass, 5.0 / cam.zoom,
            dctx); // in world coordiantes try smth like 200.0
    }
}
