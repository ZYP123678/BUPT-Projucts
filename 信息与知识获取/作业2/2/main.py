import gradio as gr
from process import *
from search import *
import time
import re
import html
import os

# 全局变量存储加载的数据和用户反馈
files = None
text_list = None
bag = None
count = None
inverse_index = None
feedback = {}  # 文件路径到评估结果的映射 (1=相关, 0=不相关)
feedback_features = {}  # 存储反馈特征向量
feedback_terms = set()
url_map = {}  # 文件路径到URL的映射
photo_metadata = None  # 存储图片元数据
adaptive_params = AdaptiveBM25Parameters()

# 存储当前搜索结果和页码
current_results = []
current_page = 0
current_query = ""  # 存储当前查询词，用于高亮

# 存储当前图片搜索结果
current_photo_results = []

# 评估统计
evaluation_stats = {
    'relevant': 0,       # 相关结果数
    'not_relevant': 0,   # 不相关结果数
    'total_evaluated': 0 # 总评估数
}

def initialize_system():
    """初始化检索系统，加载数据和创建索引"""
    global files, text_list, bag, count, inverse_index, url_map, photo_metadata
    
    status_text = "正在加载BBC新闻数据...\n"
    
    # 获取文本列表和文件路径
    start_time = time.time()
    files = get_file_paths()
    
    # 使用带OCR的文本列表
    status_text += "处理图片数据...\n"
    photo_metadata = process_photos()
    status_text += f"成功处理 {len(photo_metadata)} 张图片的文本\n"
    
    # 使用包含OCR的文本列表
    text_list = get_text_list_with_ocr()
    
    if not files or not text_list:
        return status_text + "错误: 未找到BBC新闻数据！请确保bbc_articles目录中包含新闻文件。"
        
    status_text += f"成功加载 {len(text_list)} 篇文档 (用时: {time.time()-start_time:.2f}秒)\n"
    
    # 加载URL映射
    start_time = time.time()
    status_text += "加载文章链接信息...\n"
    url_map = create_filepath_to_url_map()
    status_text += f"成功加载 {len(url_map)} 个链接 (用时: {time.time()-start_time:.2f}秒)\n"
    
    # 检查是否从缓存加载
    start_time = time.time()
    cache_exists = os.path.exists(BAG_CACHE) and os.path.exists(COUNT_CACHE)
    status_text += f"{'从缓存加载' if cache_exists else '构建新的'}词袋模型...\n"
    
    try:
        bag, count = get_bag(text_list)
        status_text += f"词袋模型包含 {len(bag.get_feature_names_out())} 个特征词 (用时: {time.time()-start_time:.2f}秒)\n"
    except Exception as e:
        return status_text + f"构建词袋模型失败: {e}"
    
    # 检查是否从缓存加载索引
    start_time = time.time()
    index_cache_exists = os.path.exists(INDEX_CACHE)
    status_text += f"{'从缓存加载' if index_cache_exists else '生成新的'}倒排索引...\n"
    
    inverse_index = generate_inverse_index(text_list, bag, count)
    index_time = time.time() - start_time
    status_text += f"索引{'加载' if index_cache_exists else '构建'}完成！共为 {len(inverse_index)} 个词条创建了索引 (用时 {index_time:.2f}秒)\n"
    
    status_text += "\n系统就绪！请在搜索框中输入查询内容"
    return status_text

def search_documents(query):
    """执行搜索并返回结果总数和第一篇文章"""
    global current_results, current_page, current_query, adaptive_params
    
    if not query.strip():
        return "请输入有效的查询内容", "0/0"
    
    if inverse_index is None:
        return "系统未初始化，请刷新页面重试", "0/0"
    
    # 保存当前查询词以便高亮显示
    current_query = query.lower()
    
    # 获取当前用户的BM25参数
    user_params = adaptive_params.get_params()
    
    # 如果有足够的反馈数据，尝试更新参数
    param_update_info = ""
    if len(feedback) >= 3 and evaluation_stats['relevant'] > 0 and evaluation_stats['not_relevant'] > 0:
        try:
            updated_params = adaptive_params.update_params("default", feedback, text_list)
            if updated_params:
                user_params = updated_params
                param_update_info = f"""<div style="background-color: #E5F6FF; padding: 10px; border-radius: 5px; margin-bottom: 15px;">
                    <b>✓ 已根据您的反馈更新检索参数</b><br>
                    k1: {updated_params['k1']:.2f} (词频饱和控制)<br>
                    b: {updated_params['b']:.2f} (文档长度归一化)<br>
                    权重: 相似度({updated_params['similarity_weight']:.1f}), BM25({updated_params['bm25_weight']:.1f}), 匹配词数({updated_params['count_weight']:.1f})
                </div>"""
        except Exception as e:
            print(f"更新参数时出错: {e}")
    
    
    # 执行搜索 - 传递反馈信息和自适应参数
    start_time = time.time()
    results = run_search(
        current_query,
        inverse_index, 
        files, 
        text_list, 
        bag, 
        count, 
        feedback=feedback, 
        feedback_features=feedback_features, 
        feedback_terms=feedback_terms,
        bm25_params=user_params  # 传递自适应参数
    )
    search_time = time.time() - start_time
    
    if not results:
        return f"未找到与'{query}'相关的内容，请尝试其他关键词", "0/0"
    
    # 保存当前结果列表和重置页码
    current_results = results
    current_page = 0
    
    # 获取第一篇结果
    result_display = format_article(0, search_time)
    
    # 添加参数更新信息
    result_display = param_update_info  + result_display
    
    # 如果是首次搜索且没有足够反馈，添加提示信息
    if evaluation_stats['total_evaluated'] < 3:
        result_display = f"""<div style="background-color: #FFF8E1; padding: 10px; border-radius: 5px; margin-bottom: 15px;">
            <b>提示:</b> 您可以通过"评估为相关"或"评估为不相关"按钮对搜索结果进行评价，系统将学习您的偏好，自动调整搜索参数和结果排序。
        </div>""" + result_display
    
    return result_display, f"1/{len(results)}"

def search_images(query):
    """搜索图片OCR内容"""
    global current_photo_results, photo_metadata

    if not query.strip():
        return "请输入有效的查询内容"
    
    # 确保图片元数据已加载
    if photo_metadata is None:
        try:
            photo_metadata = process_photos()
        except Exception as e:
            return f"加载图片数据失败: {e}"
    
    # 获取文章元数据以关联图片和文章
    article_metadata = get_metadata()
    
    # 执行图片搜索
    start_time = time.time()
    results = search_photos_by_ocr(query, photo_metadata, article_metadata)
    search_time = time.time() - start_time
    
    # 保存当前图片结果
    current_photo_results = results
    
    if not results:
        return f"<p>未找到文本包含 '{query}' 的图片，请尝试其他关键词</p>"
    
    # 生成HTML显示
    output = f"<div style='padding: 15px;'>"
    output += f"<h3>找到 {len(results)} 张文本包含 '{query}' 的图片 (搜索用时: {search_time:.2f}秒)</h3>"
    
    # 显示图片结果（添加高亮处理）
    for result in results:
        # 使用自定义高亮而不是默认的to_html方法
        output += generate_photo_result_html(result, query)
    
    output += "</div>"
    return output

def generate_photo_result_html(photo_result, query):
    """为图片搜索结果生成HTML，包括OCR文本高亮"""
    try:
        # 生成图片的base64编码（小型预览版本）
        try:
            with open(photo_result.photo_path, 'rb') as f:
                img_data = f.read()
                img_type = photo_result.photo_path.split('.')[-1].lower()
                if img_type == 'jpg' or img_type == 'jpeg':
                    mime_type = 'image/jpeg'
                elif img_type == 'png':
                    mime_type = 'image/png'
                elif img_type == 'gif':
                    mime_type = 'image/gif'
                else:
                    mime_type = 'image/jpeg'  # 默认类型
                
                # 创建缩略图
                img = Image.open(BytesIO(img_data))
                img.thumbnail((300, 300))
                buffer = BytesIO()
                img.save(buffer, format=img_type.upper())
                base64_data = base64.b64encode(buffer.getvalue()).decode('utf-8')
                img_src = f"data:{mime_type};base64,{base64_data}"
        except Exception as e:
            print(f"处理图片 {photo_result.photo_path} 失败: {e}")
            img_src = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAASwAAAEsCAYAAAB5fY51AAAACXBIWXMAAA7EAAAOxAGVKw4bAAADsklEQVR4nO3BMQEAAADCoPVPbQwfoAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAgJcBEPgAAcbL4cMAAAAASUVORK5CYII="
        
        # 生成高亮的OCR文本
        ocr_text = photo_result.ocr_text
        highlighted_ocr = highlight_keywords(ocr_text[:500] + "..." if len(ocr_text) > 500 else ocr_text, query, is_ocr=True)
        
        # 创建HTML
        html = f"""
        <div style="border: 1px solid #ddd; border-radius: 8px; overflow: hidden; margin-bottom: 15px; background: white;">
            <div style="display: flex;">
                <div style="flex: 0 0 300px;">
                    <img src="{img_src}" style="width: 100%; height: 250px; object-fit: contain; padding: 10px;" 
                         onclick="window.open('{photo_result.photo_path}', '_blank');" />
                </div>
                <div style="flex: 1; padding: 15px; overflow: hidden;">
                    <h3 style="margin-top: 0; color: #333;">{os.path.basename(photo_result.photo_path)}</h3>
                    <p style="font-size: 13px; color: #777;">相关度得分: {photo_result.score:.1f}</p>
                    <div style="background: #f5f5f5; padding: 10px; border-radius: 5px; max-height: 150px; overflow-y: auto;">
                        <p style="font-size: 13px; margin: 0;"><b>OCR文本:</b> {highlighted_ocr}</p>
                    </div>
        """
        
        # 如果有相关文章，添加链接
        if photo_result.related_article:
            article_title = photo_result.related_article.get('title', '相关文章')
            article_url = photo_result.related_article.get('url', '')
            html += f"""
                    <div style="margin-top: 10px;">
                        <p style="font-size: 13px; margin: 0;"><b>关联文章:</b> {article_title}</p>
                        {f'<a href="{article_url}" target="_blank" style="font-size: 12px; color: #0066cc;">查看文章</a>' if article_url else ''}
                    </div>
            """
        
        html += """
                </div>
            </div>
        </div>
        """
        
        return html
    except Exception as e:
        print(f"生成图片结果HTML失败: {e}")
        return f"<div>处理图片结果时出错: {e}</div>"

def highlight_keywords(text, keywords, is_ocr=False):
    """高亮文本中的关键词，包括短语"""
    if not text or not keywords:
        return html.escape(text) if text else ""
    
    try:
        # 转义HTML特殊字符
        escaped_text = html.escape(text)
        
        # OCR文本使用不同的高亮颜色
        highlight_color = "#FFEE99" if is_ocr else "#FFFF00"
        bg_color = "#F8F8E0" if is_ocr else "#FFFFCC"
        
        # 先尝试高亮完整短语
        if len(keywords.split()) > 1:
            try:
                pattern = r'\b{}\b'.format(re.escape(keywords))
                replacement = f'<span style="background-color: {highlight_color}; font-weight: bold;">{keywords}</span>'
                escaped_text = re.sub(pattern, replacement, escaped_text, flags=re.IGNORECASE)
            except Exception as e:
                print(f"短语高亮失败: {e}")
        
        # 再高亮单个词
        query_words = set(keywords.lower().split())
        for word in query_words:
            if len(word) > 2:  # 只高亮长度大于2的词
                try:
                    pattern = r'\b{}\b'.format(re.escape(word))
                    replacement = f'<span style="background-color: {bg_color};">{word}</span>'
                    escaped_text = re.sub(pattern, replacement, escaped_text, flags=re.IGNORECASE)
                except Exception as e:
                    print(f"单词高亮失败 ({word}): {e}")
        
        return escaped_text
    except Exception as e:
        print(f"高亮关键词时出错: {e}")
        return html.escape(text)

def format_article(index, search_time=None):
    """格式化单篇文章显示，支持关键词高亮和图片显示"""
    if not current_results or index < 0 or index >= len(current_results):
        return "没有可显示的文章"
    
    item = current_results[index]
    
    output = "<div style='font-family: Arial, sans-serif; line-height: 1.6;'>"
    if search_time is not None and index == 0:
        output += f"<p>找到 {len(current_results)} 条相关结果 (搜索用时: {search_time:.2f}秒)</p>"
    
    output += f"<h2>{html.escape(item.title)}</h2>"
    output += f"<p><b>类型:</b> {'视频' if item.is_video else '文章'}</p>"
    
    # 改进链接显示 - 使用url_map查找链接
    file_path = files[item.index] if item.index < len(files) else None
    if file_path and file_path in url_map and url_map[file_path].strip():
        url = url_map[file_path]
        output += f"<p><b>链接:</b> <a href='{html.escape(url)}' target='_blank'>{html.escape(url)}</a></p>"
    elif hasattr(item, 'url') and item.url and item.url.strip():
        # 如果ResultItem对象本身有链接，作为备选
        output += f"<p><b>链接:</b> <a href='{html.escape(item.url)}' target='_blank'>{html.escape(item.url)}</a></p>"
    else:
        output += "<p><b>链接:</b> [未提供]</p>"

    # 显示相关度指标
    if hasattr(item, 'total_relevance'):
        output += f"<p><b>总体相关度:</b> <span style='color: {'green' if item.total_relevance > 70 else 'orange' if item.total_relevance > 40 else 'red'}; font-weight: bold;'>{item.total_relevance:.1f}/100</span></p>"
    else:
        # 如果没有计算总体相关度，计算一个估计值 - 基于BM25的评估
        estimated_relevance = min(100, (item.similarity * 25 + item.count * 5 + item.rank * 15) * 100 / 200)
        output += f"<p><b>估计相关度:</b> <span style='color: {'green' if estimated_relevance > 70 else 'orange' if estimated_relevance > 40 else 'red'}; font-weight: bold;'>{estimated_relevance:.1f}/100</span></p>"
    
    if hasattr(item, 'has_feedback') and item.has_feedback:
        output += f"<p><b style='color: blue;'>✓ 基于您的评价调整了排名</b></p>"
    elif hasattr(item, 'feedback_adjusted') and item.feedback_adjusted:
        output += f"<p><b style='color: purple;'>✓ 根据您的反馈相似性调整了排名</b></p>"
        
    output += f"<p><b>详细指标:</b> BM25评分: {item.rank:.2f}, 匹配词数: {item.count}, 词频: {item.freq:.2f}</p>"
    output += f"<p><b>余弦相似度:</b> {item.similarity:.4f}</p>"
    
    # 添加额外的BM25特定指标说明
    if hasattr(item, 'bm25_score') and item.bm25_score != item.rank:
        output += f"<p><b>原始BM25分数:</b> {item.bm25_score:.4f}</p>"
    
    # 显示短语匹配标记
    if hasattr(item, 'is_phrase_match') and item.is_phrase_match:
        output += f"<p><b style='color: green;'>✓ 短语精确匹配</b></p>"
    
    # 显示OCR匹配标记
    if hasattr(item, 'has_ocr_match') and item.has_ocr_match:
        output += f"<p><b style='color: purple;'>✓ 图片文本匹配</b></p>"
    
    output += "<h3>内容片段:</h3>"
    
    # 显示匹配上下文（带高亮）
    shown_contexts = set()
    context_count = 0
    
    try:
        for j in item.occurrence[:5]:  # 显示前5个匹配位置
            try:
                # 确保索引在有效范围内
                if isinstance(j, tuple) and len(j) >= 2:
                    start = max(0, j[0] - 50)
                    end = min(len(item.text), j[1] + 50)
                    if start < end:  # 确保有效的片段
                        context = item.text[start:end]
                        context_hash = hash(context)
                        if context_hash not in shown_contexts:
                            # 高亮关键词
                            highlighted_context = highlight_keywords(context, current_query)
                            output += f"<blockquote>...{highlighted_context}...</blockquote>"
                            shown_contexts.add(context_hash)
                            context_count += 1
            except (IndexError, TypeError) as e:
                continue  # 跳过无效的位置索引
    except Exception as e:
        output += f"<p>显示匹配上下文时出错: {e}</p>"
    
    # 如果没有找到匹配上下文，显示文章开头
    if context_count == 0:
        try:
            preview = item.text[:300] + "..." if len(item.text) > 300 else item.text
            highlighted_preview = highlight_keywords(preview, current_query)
            output += f"<blockquote>{highlighted_preview}</blockquote>"
        except Exception as e:
            output += f"<p>显示预览时出错: {e}</p>"
    
    # 如果有OCR文本匹配，显示OCR内容
    if hasattr(item, 'ocr_text') and item.ocr_text and hasattr(item, 'has_ocr_match') and item.has_ocr_match:
        output += f"<h3>图片文本匹配:</h3>"
        highlighted_ocr = highlight_keywords(item.ocr_text[:500] + "..." if len(item.ocr_text) > 500 else item.ocr_text, current_query, is_ocr=True)
        output += f"<blockquote style='background-color: #f8f8f8;'>{highlighted_ocr}</blockquote>"
    
    # 查找并显示关联图片
    try:
        # 获取文章元数据
        article_metadata = get_metadata()
        related_photos = []
        
        # 查找当前文章关联的图片
        for metadata_item in article_metadata:
            if metadata_item['file_path'] == file_path:
                related_photos = metadata_item.get('related_photos', [])
                break
        
        # 显示关联图片
        if related_photos:
            output += "<h3>相关图片:</h3>"
            output += "<div style='display: flex; flex-wrap: wrap; gap: 15px; margin-bottom: 20px;'>"
            
            for photo_path in related_photos[:3]:  # 最多显示3张图片
                # 尝试查找图片的OCR文本
                ocr_text = ""
                for photo in photo_metadata:
                    if photo['photo_path'] == photo_path:
                        ocr_text = photo.get('ocr_text', '')
                        break
                
                # 图片显示
                try:
                    with open(photo_path, 'rb') as f:
                        img_data = f.read()
                        img_type = photo_path.split('.')[-1].lower()
                        if img_type == 'jpg' or img_type == 'jpeg':
                            mime_type = 'image/jpeg'
                        elif img_type == 'png':
                            mime_type = 'image/png'
                        elif img_type == 'gif':
                            mime_type = 'image/gif'
                        else:
                            mime_type = 'image/jpeg'
                        
                        # 创建缩略图
                        img = Image.open(BytesIO(img_data))
                        img.thumbnail((300, 300))
                        buffer = BytesIO()
                        img.save(buffer, format=img_type.upper())
                        base64_data = base64.b64encode(buffer.getvalue()).decode('utf-8')
                        img_src = f"data:{mime_type};base64,{base64_data}"
                        
                        # 显示图片卡片
                        output += f"""
                        <div style='width: 280px; border: 1px solid #ddd; border-radius: 8px; overflow: hidden;'>
                            <img src="{img_src}" style="width: 100%; height: 200px; object-fit: contain; cursor: pointer;" 
                                 onclick="window.open('{photo_path}', '_blank');" />
                            <div style='padding: 10px;'>
                                <p style='font-size: 12px; margin: 0;'><b>{os.path.basename(photo_path)}</b></p>
                        """
                        
                        # 如果有OCR文本，显示预览
                        if ocr_text:
                            ocr_preview = ocr_text[:150] + "..." if len(ocr_text) > 150 else ocr_text
                            highlighted_preview = highlight_keywords(ocr_preview, current_query, is_ocr=True)
                            output += f"""
                                <p style='font-size: 11px; color: #666; margin-top: 5px;'><b>文本:</b></p>
                                <div style='font-size: 11px; background: #f5f5f5; padding: 5px; border-radius: 4px; max-height: 80px; overflow-y: auto;'>
                                    {highlighted_preview}
                                </div>
                            """
                        
                        output += "</div></div>"
                except Exception as e:
                    print(f"处理图片 {photo_path} 失败: {e}")
            
            output += "</div>"
    except Exception as e:
        print(f"显示关联图片时出错: {e}")
    
    output += "</div>"
    return output

# 添加到main.py文件适当位置

def reset_feedback():
    """重置所有用户反馈和参数设置"""
    global feedback, feedback_features, feedback_terms, evaluation_stats, adaptive_params, query_settings

    feedback = {}
    feedback_features = {}
    feedback_terms = set()
    evaluation_stats = {'relevant': 0, 'not_relevant': 0, 'total_evaluated': 0}
    adaptive_params = AdaptiveBM25Parameters()
    query_settings = {}

    return """
所有反馈评价、参数调整和学习效果已清除。
系统恢复到默认参数状态。
"""

def next_page():
    """显示下一篇文章"""
    global current_page
    
    if not current_results:
        return "请先执行搜索", "0/0"
    
    # 移动到下一页
    if current_page < len(current_results) - 1:
        current_page += 1
    
    # 返回当前文章和页码
    return format_article(current_page), f"{current_page+1}/{len(current_results)}"

def prev_page():
    """显示上一篇文章"""
    global current_page
    
    if not current_results:
        return "请先执行搜索", "0/0"
    
    # 移动到上一页
    if current_page > 0:
        current_page -= 1
    
    # 返回当前文章和页码
    return format_article(current_page), f"{current_page+1}/{len(current_results)}"

def evaluate_relevance(is_relevant):
    """对当前文章的相关性进行评估，并提取特征用于后续检索优化"""
    global evaluation_stats, feedback, feedback_features, feedback_terms, adaptive_params
    
    if not current_results or current_page < 0 or current_page >= len(current_results):
        return "<p>请先执行搜索并浏览文章</p>"
    
    # 获取当前选中的结果项
    selected_result = current_results[current_page]
    doc_index = selected_result.index
    
    # 更新评估统计
    if is_relevant:
        evaluation_stats['relevant'] += 1
        feedback[doc_index] = 1  # 标记为相关
        result = f"<p>已将文档《{selected_result.title}》评估为【相关】</p>"
        
            
            
    else:
        evaluation_stats['not_relevant'] += 1
        feedback[doc_index] = 0  # 标记为不相关
        result = f"<p>已将文档《{selected_result.title}》评估为【不相关】</p>"
        
        # 为不相关文档提取特征(用于后续降低不相关文档的权重)
        try:
            doc_vector = count[doc_index].A[0]
            
            # 如果是首次添加负面特征，初始化特征向量
            if 'negative' not in feedback_features:
                feedback_features['negative'] = np.zeros(doc_vector.shape)
                feedback_features['count_neg'] = 0
            
            # 累加负面特征向量并增加计数
            feedback_features['negative'] += doc_vector
            feedback_features['count_neg'] += 1
        except Exception as e:
            print(f"提取负面文档特征时出错: {e}")
    
    evaluation_stats['total_evaluated'] += 1
    
    # 计算准确率
    precision = evaluation_stats['relevant'] / evaluation_stats['total_evaluated'] if evaluation_stats['total_evaluated'] > 0 else 0
    
    # 尝试更新BM25参数 - 当积累了足够的反馈时
    param_update_info = ""
    if len(feedback) >= 3 and evaluation_stats['relevant'] > 0 and evaluation_stats['not_relevant'] > 0:
        try:
            # 每3个评估或当正反馈和负反馈都有时尝试更新参数
            if evaluation_stats['total_evaluated'] % 3 == 0 or (evaluation_stats['relevant'] == 1 or evaluation_stats['not_relevant'] == 1):
                updated_params = adaptive_params.update_params("default", feedback, text_list)
                if updated_params:
                    param_update_info = f"""
<div style="background-color: #E5F6FF; padding: 10px; border-radius: 5px; margin-top: 10px;">
<b>✓ 检索参数已根据您的反馈自动更新</b>
<ul>
<li>k1: {updated_params['k1']:.2f} (控制词频重要性)</li>
<li>b: {updated_params['b']:.2f} (文档长度归一化)</li>
<li>权重分配: 相似度({updated_params['similarity_weight']:.1f}), BM25({updated_params['bm25_weight']:.1f}), 匹配词({updated_params['count_weight']:.1f})</li>
</ul>
</div>"""
        except Exception as e:
            print(f"更新BM25参数时出错: {e}")
    
    # 返回评估结果和当前准确率，同时提示用户反馈已记录
    feedback_message = ""
    if len(feedback_terms) > 0:
        top_terms = list(feedback_terms)[:5]
        feedback_message = f"""
<div style="background-color: #F0F7FF; padding: 10px; border-radius: 5px; margin-top: 10px;">
<b>✓ 系统已记录您的反馈</b>
</div>"""
    
    # 添加建议 - 当评估次数不足时
    suggestion = ""
    if evaluation_stats['total_evaluated'] < 5:
        remaining = 5 - evaluation_stats['total_evaluated']
        suggestion = f"""
<div style="background-color: #FFF8E1; padding: 10px; border-radius: 5px; margin-top: 10px;">
<b>提示:</b> 再评估{remaining}篇文档可以激活更强大的参数自适应功能。您可以继续浏览更多结果并评价它们的相关性。
</div>"""
    
    html_output = f"""
<div>
{result}
<p>当前评估统计: 相关 {evaluation_stats['relevant']}，不相关 {evaluation_stats['not_relevant']}，准确率 {precision:.2%}</p>
{param_update_info}
{feedback_message}
{suggestion}
</div>
"""
    return html_output

def check_cache_status():
    """检查系统缓存状态"""
    cache_files = {
        "词袋模型": os.path.exists(BAG_CACHE),
        "词频矩阵": os.path.exists(COUNT_CACHE),
        "倒排索引": os.path.exists(INDEX_CACHE),
        "元数据缓存": os.path.exists(METADATA_CACHE),
        "URL映射": os.path.exists(URL_MAP_CACHE),
        "图片文本结果缓存": os.path.exists(OCR_CACHE),
        "图片元数据缓存": os.path.exists(PHOTO_METADATA_CACHE)
    }
    
    status = "系统缓存状态:\n"
    for name, exists in cache_files.items():
        status += f"✓ {name}: {'已缓存' if exists else '未缓存'}\n"
    
    # 图片数量统计
    photo_count = len(get_photo_paths()) if os.path.exists(photos_path) else 0
    status += f"\n图片目录中共有 {photo_count} 张图片"
    
    return status

# 创建Gradio界面
def create_interface():
    with gr.Blocks(title="BBC新闻多媒体检索系统") as demo:
        gr.Markdown("# BBC 新闻多媒体检索系统")
        gr.Markdown("### 集成文本和图片的智能新闻检索平台")
        
        with gr.Tabs() as tabs:
            # 文章检索标签页
            with gr.TabItem("文章检索"):
                with gr.Row():
                    with gr.Column(scale=1):
                        # 系统控制区域
                        with gr.Row():
                            init_button = gr.Button("初始化系统", variant="primary")
                            check_cache_button = gr.Button("检查缓存状态")
                            
                        status_output = gr.Textbox(label="系统状态", lines=10)
                        
                        # 缓存管理
                        with gr.Row():
                            clear_cache_button = gr.Button("清除缓存（数据更新后使用）", variant="stop")
                        
                        # 搜索区域
                        query_input = gr.Textbox(label="输入查询内容", placeholder="例如: climate change")
                        search_button = gr.Button("搜索", variant="primary")
                        
                        # 页码显示
                        page_info = gr.Textbox(label="页码", value="0/0")
                        
                        # 翻页和反馈区域
                        with gr.Row():
                            prev_button = gr.Button("← 上一篇")
                            next_button = gr.Button("下一篇 →")
                        
                        # 评估按钮
                        with gr.Row():
                            relevant_button = gr.Button("✓ 评估为相关", variant="secondary")
                            not_relevant_button = gr.Button("✗ 评估为不相关", variant="secondary")
                        
                        evaluation_output = gr.HTML(label="评估结果")
                        
                        with gr.Row():
                            reset_button = gr.Button("🔄 重置所有反馈", variant="secondary")

                        # 在事件处理部分添加
                        reset_button.click(
                            reset_feedback,
                            inputs=[],
                            outputs=evaluation_output
                        )
                        
                    with gr.Column(scale=2):
                        # 结果显示区域 - 使用HTML组件支持高亮
                        results_output = gr.HTML(label="文章内容", elem_id="article-results")
            
            # 图片搜索标签页
            with gr.TabItem("图片搜索"):
                with gr.Row():
                    with gr.Column(scale=1):
                        # 图片搜索区域
                        image_query_input = gr.Textbox(
                            label="搜索图片文本", 
                            placeholder="输入要在图片中查找的文字...",
                            info="搜索图片"
                        )
                        image_search_button = gr.Button("搜索图片", variant="primary")
                        
                        gr.Markdown("""
                        ### 图片搜索说明
                        
                        系统会识别图片中的文字内容，您可以：
                        - 搜索图片中出现的特定文字或短语
                        - 查看识别出的完整文本
                        - 查看图片所关联的新闻文章
                        
                        文本质量取决于图片清晰度和文字特征。
                        """)
                        
                    with gr.Column(scale=2):
                        # 图片搜索结果显示
                        image_results_output = gr.HTML(label="图片搜索结果", elem_id="image-results")
        
        # 添加自定义CSS
        gr.HTML("""
        <style>
            #article-results {
                min-height: 600px;
                overflow-y: auto;
            }
            #image-results {
                min-height: 600px;
                overflow-y: auto;
            }
            blockquote {
                border-left: 3px solid #ccc;
                padding-left: 10px;
                margin: 10px 0;
                background-color: #f9f9f9;
            }
            img {
                border-radius: 5px;
                box-shadow: 0 2px 5px rgba(0,0,0,0.1);
            }
        </style>
        """)
                
        # 设置事件处理
        # 系统初始化和缓存管理
        init_button.click(initialize_system, inputs=[], outputs=status_output)
        check_cache_button.click(check_cache_status, inputs=[], outputs=status_output)
        clear_cache_button.click(
            lambda: (clear_cache(), "缓存已清除，下次初始化时将重建索引"),
            inputs=[],
            outputs=status_output
        )
        
        # 文章搜索功能
        search_button.click(search_documents, inputs=query_input, outputs=[results_output, page_info])
        
        # 翻页按钮
        next_button.click(next_page, inputs=[], outputs=[results_output, page_info])
        prev_button.click(prev_page, inputs=[], outputs=[results_output, page_info])
        
        # 评估按钮
        relevant_button.click(
            lambda: evaluate_relevance(True), 
            inputs=[], 
            outputs=evaluation_output
        )
        not_relevant_button.click(
            lambda: evaluate_relevance(False), 
            inputs=[], 
            outputs=evaluation_output
        )
        
        # 图片搜索功能
        image_search_button.click(
            search_images,
            inputs=image_query_input,
            outputs=image_results_output
        )
        
        # 自动初始化系统
        demo.load(check_cache_status, inputs=[], outputs=status_output)
    
    return demo

# 启动应用
if __name__ == "__main__":
    interface = create_interface()
    interface.launch(share=False)