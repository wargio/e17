/*============================================================================*
 *                                 Blend                                      *
 *============================================================================*/
static void argb8888_pt_color_blend_c(Enesim_Surface_Data *d,
		Enesim_Surface_Pixel *s, Enesim_Surface_Pixel *color,
		Enesim_Surface_Pixel *m)
{
	uint16_t a;

	a = 256 - argb8888_alpha_get(color->pixel.argb8888.plane0);
	argb8888_blend(d->data.argb8888.plane0, a, color->pixel.argb8888.plane0);
}

static void argb8888_pt_mask_color_blend_argb8888_c(Enesim_Surface_Data *d,
		Enesim_Surface_Pixel *s, Enesim_Surface_Pixel *color,
		Enesim_Surface_Pixel *m)
{
	uint16_t ca = 256 - argb8888_alpha_get(color->pixel.argb8888.plane0);
	uint16_t ma = argb8888_alpha_get(m->pixel.argb8888.plane0);

	switch (ma)
	{
		case 0:
		break;
	
		case 255:
		argb8888_blend(d->data.argb8888.plane0, ca, color->pixel.argb8888.plane0);
		break;
		
		default:
		{
			uint32_t mc;
			
			mc = argb8888_mul_sym(ma, color->pixel.argb8888.plane0);
			ma = 256 - argb8888_alpha_get(mc);
			argb8888_blend(d->data.argb8888.plane0, ma, mc);
		}
		break;
	}
}

static void argb8888_pt_mask_color_blend_a8_c(Enesim_Surface_Data *d,
		Enesim_Surface_Pixel *s, Enesim_Surface_Pixel *color,
		Enesim_Surface_Pixel *m)
{
	
}

static void argb8888_pt_pixel_blend_argb8888_c(Enesim_Surface_Data *d,
		Enesim_Surface_Pixel *s, Enesim_Surface_Pixel *color,
		Enesim_Surface_Pixel *m)
{
	uint16_t a;

	a = 256 - argb8888_alpha_get(s->pixel.argb8888.plane0);
	argb8888_blend(d->data.argb8888.plane0, a, s->pixel.argb8888.plane0);
}

static void argb8888_pt_pixel_blend_generic_c(Enesim_Surface_Data *d,
		Enesim_Surface_Pixel *s, Enesim_Surface_Pixel *color,
		Enesim_Surface_Pixel *m)
{
	uint32_t argb;
	uint16_t a;

	argb = enesim_surface_pixel_argb_to(s);
	a = 256 - argb8888_alpha_get(argb);
	argb8888_blend(d->data.argb8888.plane0, a, argb);
}

static void argb8888_sp_color_blend_c(Enesim_Surface_Data *d,
		unsigned int len, Enesim_Surface_Data *s,
		Enesim_Surface_Pixel *color, Enesim_Surface_Data *m)
{
	Enesim_Surface_Data dtmp, end;
	uint16_t a;

	argb8888_data_copy(d, &dtmp);
	argb8888_data_offset(d, &end, len);
	a = 256 - argb8888_alpha_get(color->pixel.argb8888.plane0);
	while (dtmp.data.argb8888.plane0 < end.data.argb8888.plane0)
	{
		argb8888_blend(dtmp.data.argb8888.plane0, a, color->pixel.argb8888.plane0);
		argb8888_data_increment(&dtmp, 1);
	}
}
static void argb8888_sp_pixel_blend_argb8888_c(Enesim_Surface_Data *d,
		unsigned int len, Enesim_Surface_Data *s,
		Enesim_Surface_Pixel *color, Enesim_Surface_Data *m)
{
	Enesim_Surface_Data stmp, dtmp, end;

	argb8888_data_copy(d, &dtmp);
	argb8888_data_copy(s, &stmp);
	argb8888_data_offset(d, &end, len);
	while (dtmp.data.argb8888.plane0 < end.data.argb8888.plane0)
	{
		uint16_t a;

		a = 256 - argb8888_alpha_get(*stmp.data.argb8888.plane0);
		argb8888_blend(dtmp.data.argb8888.plane0, a, *stmp.data.argb8888.plane0);
		argb8888_data_increment(&stmp, 1);
		argb8888_data_increment(&dtmp, 1);
	}
}

static void argb8888_sp_mask_color_blend_argb8888_c(Enesim_Surface_Data *d, unsigned int len,
		Enesim_Surface_Data *s, Enesim_Surface_Pixel *color,
		Enesim_Surface_Data *m)
{
	Enesim_Surface_Data dtmp, mtmp, end;
	uint16_t ca = 256 - argb8888_alpha_get(color->pixel.argb8888.plane0);

	argb8888_data_copy(d, &dtmp);
	argb8888_data_copy(m, &mtmp);
	argb8888_data_offset(d, &end, len);
	while (dtmp.data.argb8888.plane0 < end.data.argb8888.plane0)
	{
		uint16_t ma = argb8888_alpha_get(*mtmp.data.argb8888.plane0);

		switch (ma)
		{
			case 0:
			break;

			case 255:
			argb8888_blend(dtmp.data.argb8888.plane0, ca, color->pixel.argb8888.plane0);
			break;
			
			default:
			{
				uint32_t mc;

				mc = argb8888_mul_sym(ma, color->pixel.argb8888.plane0);
				ma = 256 - argb8888_alpha_get(mc);
				argb8888_blend(dtmp.data.argb8888.plane0, ma, mc);
			}
			break;
		}
		argb8888_data_increment(&mtmp, 1);
		argb8888_data_increment(&dtmp, 1);
	}
}

static void argb8888_sp_pixel_mask_blend_argb8888_argb8888_c(Enesim_Surface_Data *d, unsigned int len,
		Enesim_Surface_Data *s, Enesim_Surface_Pixel *color,
		Enesim_Surface_Data *m)
{
	Enesim_Surface_Data dtmp, stmp, mtmp, end;

	argb8888_data_copy(d, &dtmp);
	argb8888_data_copy(m, &mtmp);
	argb8888_data_copy(s, &stmp);
	argb8888_data_offset(d, &end, len);
	while (dtmp.data.argb8888.plane0 < end.data.argb8888.plane0)
	{
		uint16_t ma = argb8888_alpha_get(*mtmp.data.argb8888.plane0);

		switch (ma)
		{
			case 0:
			break;

			case 255:
			{
				uint16_t sa;
				
				sa = 256 - argb8888_alpha_get(*stmp.data.argb8888.plane0);
				argb8888_blend(dtmp.data.argb8888.plane0, sa, *stmp.data.argb8888.plane0);
			}
			break;
			
			default:
			{
				uint32_t mc;

				mc = argb8888_mul_sym(ma, *stmp.data.argb8888.plane0);
				ma = 256 - argb8888_alpha_get(mc);
				argb8888_blend(dtmp.data.argb8888.plane0, ma, mc);
			}
			break;
		}
		argb8888_data_increment(&mtmp, 1);
		argb8888_data_increment(&stmp, 1);
		argb8888_data_increment(&dtmp, 1);
	}
}



static void argb8888_sp_pixel_blend_generic_c(Enesim_Surface_Data *d,
		unsigned int len, Enesim_Surface_Data *s,
		Enesim_Surface_Pixel *color, Enesim_Surface_Data *m)
{
	Enesim_Surface_Data stmp, dtmp, end;

	argb8888_data_copy(d, &dtmp);
	stmp = *s;
	argb8888_data_offset(d, &end, len);
	while (dtmp.data.argb8888.plane0 < end.data.argb8888.plane0)
	{
		Enesim_Surface_Pixel p;

		enesim_surface_data_pixel_get(&stmp, &p);
		argb8888_pt_pixel_blend_generic_c(&dtmp, &p, 0, NULL);
		enesim_surface_data_increment(&stmp, 1);
		argb8888_data_increment(&dtmp, 1);
	}
}
/*============================================================================*
 *                                  Fill                                      * 
 *============================================================================*/
static void argb8888_pt_color_fill_c(Enesim_Surface_Data *d,
		Enesim_Surface_Pixel *s, Enesim_Surface_Pixel *color,
		Enesim_Surface_Pixel *m)
{
	argb8888_fill(d->data.argb8888.plane0, color->pixel.argb8888.plane0);
}
static void argb8888_pt_mask_color_fill_argb8888_c(Enesim_Surface_Data *d,
		Enesim_Surface_Pixel *s,
		Enesim_Surface_Pixel *color,
		Enesim_Surface_Pixel *m)
{
	uint16_t a = argb8888_alpha_get(m->pixel.argb8888.plane0);
	switch (a)
	{
		case 0:
		break;
		
		case 255:
		argb8888_fill(d->data.argb8888.plane0, color->pixel.argb8888.plane0);
		break;

		default:
		argb8888_fill(d->data.argb8888.plane0,
				argb8888_interp_256(a + 1,
				color->pixel.argb8888.plane0,
				*d->data.argb8888.plane0));
		break;
	}	
}

static void argb8888_pt_pixel_fill_argb8888_c(Enesim_Surface_Data *d,
		Enesim_Surface_Pixel *s,
		Enesim_Surface_Pixel *color,
		Enesim_Surface_Pixel *m)
{
	argb8888_fill(d->data.argb8888.plane0, s->pixel.argb8888.plane0);
}

static void argb8888_pt_pixel_color_fill_argb8888_c(Enesim_Surface_Data *d,
		Enesim_Surface_Pixel *s,
		Enesim_Surface_Pixel *color,
		Enesim_Surface_Pixel *m)
{
	argb8888_fill(d->data.argb8888.plane0, argb8888_mul4_sym(color->pixel.argb8888.plane0, s->pixel.argb8888.plane0));	
}


static void argb8888_pt_pixel_fill_generic_c(Enesim_Surface_Data *d,
		Enesim_Surface_Pixel *s,
		Enesim_Surface_Pixel *color,
		Enesim_Surface_Pixel *m)
{
	uint32_t argb;

	argb = enesim_surface_pixel_argb_to(s);
	argb8888_fill(d->data.argb8888.plane0, argb);
}

static void argb8888_sp_color_fill_c(Enesim_Surface_Data *d, unsigned int len,
		Enesim_Surface_Data *s, Enesim_Surface_Pixel *color,
		Enesim_Surface_Data *m)
{
	Enesim_Surface_Data dtmp, end;

	argb8888_data_copy(d, &dtmp);
	argb8888_data_offset(d, &end, len);
	while (dtmp.data.argb8888.plane0 < end.data.argb8888.plane0)
	{
		argb8888_fill(dtmp.data.argb8888.plane0, color->pixel.argb8888.plane0);
		argb8888_data_increment(&dtmp, 1);
	}
}
static void argb8888_sp_pixel_fill_argb8888_c(Enesim_Surface_Data *d,
		unsigned int len, Enesim_Surface_Data *s,
		Enesim_Surface_Pixel *color, Enesim_Surface_Data *m)
{
	Enesim_Surface_Data stmp, dtmp, end;

	argb8888_data_copy(d, &dtmp);
	argb8888_data_copy(s, &stmp);
	argb8888_data_offset(d, &end, len);
	while (dtmp.data.argb8888.plane0 < end.data.argb8888.plane0)
	{
		argb8888_fill(dtmp.data.argb8888.plane0, *stmp.data.argb8888.plane0);
		argb8888_data_increment(&stmp, 1);
		argb8888_data_increment(&dtmp, 1);
	}
}

static void argb8888_sp_mask_color_fill_argb8888_c(Enesim_Surface_Data *d, unsigned int len,
		Enesim_Surface_Data *s, Enesim_Surface_Pixel *color,
		Enesim_Surface_Data *m)
{
	Enesim_Surface_Data dtmp, mtmp, end;

	argb8888_data_copy(d, &dtmp);
	argb8888_data_copy(m, &mtmp);
	argb8888_data_offset(d, &end, len);
	while (dtmp.data.argb8888.plane0 < end.data.argb8888.plane0)
	{
		uint16_t a = argb8888_alpha_get(*mtmp.data.argb8888.plane0);
		uint32_t c;
		switch (a)
		{
			case 0:
			break;
				
			case 255:
			argb8888_fill(dtmp.data.argb8888.plane0, color->pixel.argb8888.plane0);
			break;

			default:
			c = argb8888_interp_256(a + 1, color->pixel.argb8888.plane0, *dtmp.data.argb8888.plane0);
			argb8888_fill(dtmp.data.argb8888.plane0, c);
			break;
		}
		argb8888_data_increment(&mtmp, 1);
		argb8888_data_increment(&dtmp, 1);
	}
}

static void argb8888_sp_pixel_mask_fill_argb8888_argb8888_c(Enesim_Surface_Data *d, unsigned int len,
		Enesim_Surface_Data *s, Enesim_Surface_Pixel *color,
		Enesim_Surface_Data *m)
{
	Enesim_Surface_Data dtmp, stmp, mtmp, end;

	argb8888_data_copy(d, &dtmp);
	argb8888_data_copy(s, &stmp);
	argb8888_data_copy(m, &mtmp);
	argb8888_data_offset(d, &end, len);
	while (dtmp.data.argb8888.plane0 < end.data.argb8888.plane0)
	{
		uint16_t a = argb8888_alpha_get(*mtmp.data.argb8888.plane0);
		uint32_t c;
		switch (a)
		{
			case 0:
			break;
				
			case 255:
			argb8888_fill(dtmp.data.argb8888.plane0, *stmp.data.argb8888.plane0);
			break;

			default:
			c = argb8888_interp_256(a + 1, *stmp.data.argb8888.plane0, *dtmp.data.argb8888.plane0);
			argb8888_fill(dtmp.data.argb8888.plane0, c);
			break;
		}
		argb8888_data_increment(&mtmp, 1);
		argb8888_data_increment(&stmp, 1);
		argb8888_data_increment(&dtmp, 1);
	}
}


static void argb8888_sp_pixel_fill_generic_c(Enesim_Surface_Data *d,
		unsigned int len, Enesim_Surface_Data *s,
		Enesim_Surface_Pixel *color, Enesim_Surface_Data *m)
{
	Enesim_Surface_Data stmp, dtmp, end;

	argb8888_data_copy(d, &dtmp);
	stmp = *s;
	argb8888_data_offset(d, &end, len);
	while (dtmp.data.argb8888.plane0 < end.data.argb8888.plane0)
	{
		Enesim_Surface_Pixel p;

		enesim_surface_data_pixel_get(&stmp, &p);
		argb8888_pt_pixel_fill_generic_c(&dtmp, &p, 0, NULL);
		enesim_surface_data_increment(&stmp, 1);
		argb8888_data_increment(&dtmp, 1);
	}
}

#define argb8888_pt_color_blend argb8888_pt_color_blend_c
#define argb8888_pt_mask_color_blend_argb8888 argb8888_pt_mask_color_blend_argb8888_c
#define argb8888_pt_pixel_blend_argb8888 argb8888_pt_pixel_blend_argb8888_c
#define argb8888_pt_pixel_blend_argb8888_unpre argb8888_pt_pixel_blend_generic_c
#define argb8888_pt_pixel_blend_rgb565_b1a3 argb8888_pt_pixel_blend_generic_c

#define argb8888_sp_color_blend argb8888_sp_color_blend_c
#define argb8888_sp_mask_color_blend_argb8888 argb8888_sp_mask_color_blend_argb8888_c
#define argb8888_sp_pixel_blend_argb8888 argb8888_sp_pixel_blend_argb8888_c
#define argb8888_sp_pixel_blend_argb8888_unpre argb8888_sp_pixel_blend_generic_c
#define argb8888_sp_pixel_blend_rgb565_b1a3 argb8888_sp_pixel_blend_generic_c
#define argb8888_sp_pixel_mask_blend_argb8888_argb8888 argb8888_sp_pixel_mask_blend_argb8888_argb8888_c

#define argb8888_pt_color_fill argb8888_pt_color_fill_c
#define argb8888_pt_mask_color_fill_argb8888 argb8888_pt_mask_color_fill_argb8888_c
#define argb8888_pt_pixel_color_fill_argb8888 argb8888_pt_pixel_color_fill_argb8888_c
#define argb8888_pt_pixel_fill_argb8888 argb8888_pt_pixel_fill_argb8888_c
#define argb8888_pt_pixel_fill_argb8888_unpre argb8888_pt_pixel_fill_generic_c
#define argb8888_pt_pixel_fill_rgb565_b1a3 argb8888_pt_pixel_fill_generic_c

#define argb8888_sp_color_fill argb8888_sp_color_fill_c
#define argb8888_sp_mask_color_fill_argb8888 argb8888_sp_mask_color_fill_argb8888_c
#define argb8888_sp_pixel_fill_argb8888 argb8888_sp_pixel_fill_argb8888_c
#define argb8888_sp_pixel_fill_argb8888_unpre argb8888_sp_pixel_fill_generic_c
#define argb8888_sp_pixel_fill_rgb565_b1a3 argb8888_sp_pixel_fill_generic_c
#define argb8888_sp_pixel_mask_fill_argb8888_argb8888 argb8888_sp_pixel_mask_fill_argb8888_argb8888_c

